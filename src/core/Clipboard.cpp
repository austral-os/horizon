#include "horizon/Clipboard.hpp"
#include "WaylandClipboardBackend.hpp"
#include "horizon/WaylandSurface.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/Logger.hpp"
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <algorithm>


namespace horizon {

// --- ClipboardData Implementation ---

void ClipboardData::set(const std::string& mime, const std::vector<uint8_t>& data) {
    m_data[mime] = data;
}

bool ClipboardData::has(const std::string& mime) const {
    return m_data.count(mime) > 0;
}

const std::vector<uint8_t>& ClipboardData::get(const std::string& mime) const {
    static const std::vector<uint8_t> empty;
    auto it = m_data.find(mime);
    if (it != m_data.end()) return it->second;
    return empty;
}

std::vector<std::string> ClipboardData::mime_types() const {
    std::vector<std::string> types;
    for (const auto& [mime, _] : m_data) types.push_back(mime);
    return types;
}

// --- Clipboard Implementation ---

void Clipboard::set(const ClipboardData& data) {
    auto* window = WaylandWindow::get_active_window();
    if (window) {
        window->set_clipboard_data(data);
    }
}

void Clipboard::set_text(const std::string& text) {
    ClipboardData data;
    std::vector<uint8_t> bytes(text.begin(), text.end());
    data.set("text/plain", bytes);
    data.set("text/plain;charset=utf-8", bytes);
    set(data);
}

// --- WaylandClipboardBackend Implementation ---

static const struct wl_data_source_listener data_source_listener = {
    WaylandClipboardBackend::data_source_handle_target,
    WaylandClipboardBackend::data_source_handle_send,
    WaylandClipboardBackend::data_source_handle_cancelled,
    WaylandClipboardBackend::data_source_handle_dnd_drop_performed,
    WaylandClipboardBackend::data_source_handle_dnd_finished,
    WaylandClipboardBackend::data_source_handle_action
};
 
static const struct wl_data_offer_listener data_offer_listener = {
    WaylandClipboardBackend::data_offer_handle_offer,
    nullptr, // source_actions
    nullptr  // action
};

static const struct wl_data_device_listener data_device_listener = {

    WaylandClipboardBackend::data_device_handle_data_offer,
    WaylandClipboardBackend::data_device_handle_enter,
    WaylandClipboardBackend::data_device_handle_leave,
    WaylandClipboardBackend::data_device_handle_motion,
    WaylandClipboardBackend::data_device_handle_drop,
    WaylandClipboardBackend::data_device_handle_selection
};

WaylandClipboardBackend::WaylandClipboardBackend(WaylandSurface* surface) : m_surface(surface) {
    if (m_surface && m_surface->data_device()) {
        wl_data_device_add_listener(m_surface->data_device(), &data_device_listener, this);
    } else {
        LOG_WARNING << "WaylandClipboardBackend: data_device not available during initialization";
    }
}

WaylandClipboardBackend::~WaylandClipboardBackend() {
    cleanup_source();
    cleanup_offer();
    cleanup_pending_offer();
}


void WaylandClipboardBackend::set(const ClipboardData& data) {
    if (!m_surface || !m_surface->data_device_manager() || !m_surface->data_device()) {
        LOG_ERROR << "WaylandClipboardBackend: Cannot set selection, data device not available";
        return;
    }

    cleanup_source();
    m_current_data = data;
    m_source = wl_data_device_manager_create_data_source(m_surface->data_device_manager());
    
    if (!m_source) {
        LOG_ERROR << "WaylandClipboardBackend: Failed to create wl_data_source";
        return;
    }

    LOG_INFO << "WaylandClipboardBackend: Setting selection with " << m_current_data->mime_types().size() << " mime types";

    for (const auto& mime : m_current_data->mime_types()) {
        wl_data_source_offer(m_source, mime.c_str());
    }

    wl_data_source_add_listener(m_source, &data_source_listener, this);
    wl_data_device_set_selection(m_surface->data_device(), m_source, m_surface->last_serial());
}

void WaylandClipboardBackend::cleanup_source() {
    if (m_source) {
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }
    m_current_data.reset();
}

void WaylandClipboardBackend::cleanup_offer() {
    if (m_current_offer) {
        wl_data_offer_destroy(m_current_offer);
        m_current_offer = nullptr;
    }
    m_offered_mime_types.clear();
    m_offer_counter++;
}


void WaylandClipboardBackend::cleanup_pending_offer() {
    if (m_pending_offer) {
        wl_data_offer_destroy(m_pending_offer);
        m_pending_offer = nullptr;
    }
    m_pending_mime_types.clear();
    m_offer_counter++;
}




// --- Source Listeners ---

void WaylandClipboardBackend::data_source_handle_target(void *, struct wl_data_source *, const char *) {}

void WaylandClipboardBackend::data_source_handle_send(void *data, struct wl_data_source *, const char *mime_type, int32_t fd) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    if (!self->m_current_data) {
        LOG_INFO << "WaylandClipboardBackend: data_source_handle_send called but no data available";
        close(fd);
        return;
    }

    const std::vector<uint8_t>* buffer = nullptr;
    if (self->m_current_data->has(mime_type)) {
        buffer = &self->m_current_data->get(mime_type);
    } else if (self->m_current_data->has("text/plain")) {
        buffer = &self->m_current_data->get("text/plain");
    }

    if (buffer && !buffer->empty()) {
        const uint8_t* ptr = buffer->data();
        size_t left = buffer->size();
        
        LOG_INFO << "WaylandClipboardBackend: Sending " << left << " bytes for mime " << mime_type;

        while (left > 0) {
            ssize_t ret = write(fd, ptr, left);
            if (ret > 0) {
                ptr += ret;
                left -= ret;
            } else if (ret == -1) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    struct pollfd pfd = {fd, POLLOUT, 0};
                    int poll_ret = poll(&pfd, 1, 500); // 500ms timeout
                    if (poll_ret > 0) {
                        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                            LOG_ERROR << "WaylandClipboardBackend: POLL error during send: " << pfd.revents;
                            break;
                        }
                        if (pfd.revents & POLLOUT) continue;
                    } else {
                        LOG_ERROR << "WaylandClipboardBackend: Poll timeout or error during send";
                        break;
                    }
                } else {
                    LOG_ERROR << "WaylandClipboardBackend: write error: " << strerror(errno);
                    break;
                }
            } else {
                break;
            }
        }
    }
    close(fd);
}

void WaylandClipboardBackend::data_source_handle_cancelled(void *data, struct wl_data_source *) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    LOG_INFO << "WaylandClipboardBackend: Data source cancelled";
    self->cleanup_source();
}

void WaylandClipboardBackend::data_source_handle_dnd_drop_performed(void *, struct wl_data_source *) {}
void WaylandClipboardBackend::data_source_handle_dnd_finished(void *, struct wl_data_source *) {}
void WaylandClipboardBackend::data_source_handle_action(void *, struct wl_data_source *, uint32_t) {}

// --- Device Listeners ---

void WaylandClipboardBackend::data_device_handle_data_offer(void *data, struct wl_data_device *, struct wl_data_offer *id) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    self->cleanup_pending_offer();
    self->m_pending_offer = id;
    self->m_offer_counter++;
    
    LOG_INFO << "WaylandClipboardBackend: New pending offer received: " << id;
    wl_data_offer_add_listener(id, &data_offer_listener, self);
}



void WaylandClipboardBackend::data_device_handle_enter(void *, struct wl_data_device *, uint32_t, struct wl_surface *, wl_fixed_t, wl_fixed_t, struct wl_data_offer *) {}
void WaylandClipboardBackend::data_device_handle_leave(void *, struct wl_data_device *) {}
void WaylandClipboardBackend::data_device_handle_motion(void *, struct wl_data_device *, uint32_t, wl_fixed_t, wl_fixed_t) {}
void WaylandClipboardBackend::data_device_handle_drop(void *, struct wl_data_device *) {}

void WaylandClipboardBackend::data_device_handle_selection(void *data, struct wl_data_device *, struct wl_data_offer *id) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    
    if (id == nullptr) {
        LOG_INFO << "WaylandClipboardBackend: Selection cleared";
        self->cleanup_offer();
        return;
    }
    
    if (id == self->m_pending_offer) {
        LOG_INFO << "WaylandClipboardBackend: Promoting pending offer to selection: " << id;
        self->cleanup_offer();
        self->m_current_offer = self->m_pending_offer;
        self->m_offered_mime_types = std::move(self->m_pending_mime_types);
        
        self->m_pending_offer = nullptr;
        self->m_pending_mime_types.clear();
    } else if (id != self->m_current_offer) {
        LOG_INFO << "WaylandClipboardBackend: External selection received: " << id;
        self->cleanup_offer();
        self->m_current_offer = id;
    }
}

void WaylandClipboardBackend::data_offer_handle_offer(void *data, struct wl_data_offer *offer, const char *mime_type) {
    if (!mime_type) return;
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    
    auto add_unique = [](std::vector<std::string>& vec, const std::string& mime) {
        if (std::find(vec.begin(), vec.end(), mime) == vec.end()) {
            vec.push_back(mime);
        }
    };

    if (offer == self->m_pending_offer) {
        add_unique(self->m_pending_mime_types, mime_type);
    } else if (offer == self->m_current_offer) {
        add_unique(self->m_offered_mime_types, mime_type);
    }
}


std::optional<ClipboardData> WaylandClipboardBackend::get(const std::vector<std::string>& preferred_mimes) {
    if (!m_current_offer || !m_surface) return std::nullopt;

    auto* display = m_surface->display();
    if (!display) return std::nullopt;

    wl_data_offer* offer = m_current_offer;
    int initial_counter = m_offer_counter;

    ClipboardData result;
    const size_t MAX_CLIPBOARD_SIZE = 4 * 1024 * 1024; // Increased to 4MB

    const std::vector<std::string>& target_mimes = preferred_mimes.empty() ? m_offered_mime_types : preferred_mimes;

    LOG_INFO << "WaylandClipboardBackend::get: Requesting data for " << target_mimes.size() << " mime types";

    for (const auto& mime : target_mimes) {
        if (m_offer_counter != initial_counter) {
            LOG_INFO << "WaylandClipboardBackend: Selection changed during get(), aborting";
            return std::nullopt;
        }

        bool available = false;
        if (preferred_mimes.empty()) {
            available = true;
        } else {
            for (const auto& offered : m_offered_mime_types) {
                if (offered == mime) { available = true; break; }
            }
        }
        if (!available) continue;

        LOG_INFO << "WaylandClipboardBackend::get: Requesting " << mime;
        int pipefd[2];
        
#ifdef __linux__
        if (pipe2(pipefd, O_CLOEXEC) == -1) {
            LOG_ERROR << "WaylandClipboardBackend: pipe2 failed: " << strerror(errno);
            continue;
        }
#else
        if (pipe(pipefd) == -1) {
            LOG_ERROR << "WaylandClipboardBackend: pipe failed: " << strerror(errno);
            continue;
        }
        fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
        fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
#endif

        // Verify safety before receive (Fix 8)
        if (offer != m_current_offer || m_offer_counter != initial_counter) {
            LOG_WARNING << "WaylandClipboardBackend: Offer invalidated before receive call";
            close(pipefd[0]);
            close(pipefd[1]);
            return std::nullopt;
        }

        wl_data_offer_receive(offer, mime.c_str(), pipefd[1]);
        close(pipefd[1]);

        if (m_offer_counter != initial_counter) {
            LOG_INFO << "WaylandClipboardBackend: Selection changed during receive, aborting";
            close(pipefd[0]);
            return std::nullopt;
        }

        wl_display_flush(display);
        wl_display_dispatch_pending(display);

        if (m_offer_counter != initial_counter) {
            LOG_INFO << "WaylandClipboardBackend: Selection changed during dispatch, aborting";
            close(pipefd[0]);
            return std::nullopt;
        }

        std::vector<uint8_t> data;
        uint8_t buffer[4096];
        struct pollfd pfd = {pipefd[0], POLLIN, 0};

        while (true) {
            if (m_offer_counter != initial_counter) break;

            int poll_ret = poll(&pfd, 1, 500); 
            if (poll_ret <= 0) {
                if (poll_ret == -1 && errno == EINTR) continue;
                LOG_INFO << "WaylandClipboardBackend: read timeout or error for " << mime;
                break; 
            }

            if (pfd.revents & (POLLERR | POLLNVAL)) {
                LOG_ERROR << "WaylandClipboardBackend: POLL error on read fd: " << pfd.revents;
                break;
            }

            // Handle POLLHUP as EOF (Fix 4)
            if (!(pfd.revents & POLLIN) && (pfd.revents & POLLHUP)) {
                LOG_INFO << "WaylandClipboardBackend: Received POLLHUP (EOF)";
                break;
            }

            ssize_t n = read(pipefd[0], buffer, sizeof(buffer));
            if (n > 0) {
                if (data.size() + n > MAX_CLIPBOARD_SIZE) {
                    LOG_WARNING << "WaylandClipboardBackend: Maximum clipboard size exceeded for " << mime;
                    break;
                }
                data.insert(data.end(), buffer, buffer + n);
            } else if (n == 0) {
                break; // EOF
            } else if (n == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                if (errno == EINTR) continue;
                LOG_ERROR << "WaylandClipboardBackend: Read error: " << strerror(errno);
                break;
            }
        }

        close(pipefd[0]);

        if (m_offer_counter != initial_counter) return std::nullopt;

        if (!data.empty()) {
            LOG_INFO << "WaylandClipboardBackend: Successfully retrieved " << data.size() << " bytes for " << mime;
            result.set(mime, data);
            if (!preferred_mimes.empty()) return result;
        }
    }

    return result;
}

std::optional<ClipboardData> Clipboard::get() {
    auto* window = WaylandWindow::get_active_window();
    if (window) {
        return window->get_clipboard_data();
    }
    return std::nullopt;
}

std::optional<std::string> Clipboard::get_text() {
    auto* window = WaylandWindow::get_active_window();
    if (!window) return std::nullopt;

    auto data = window->get_clipboard_data({"text/plain;charset=utf-8", "text/plain"});
    if (!data) return std::nullopt;

    std::vector<uint8_t> bytes;
    if (data->has("text/plain;charset=utf-8")) {
        bytes = data->get("text/plain;charset=utf-8");
    } else if (data->has("text/plain")) {
        bytes = data->get("text/plain");
    } else {
        return std::nullopt;
    }

    // Normalize \r\n -> \n (Fix 9)
    std::string text(bytes.begin(), bytes.end());
    size_t pos = 0;
    while ((pos = text.find("\r\n", pos)) != std::string::npos) {
        text.replace(pos, 2, "\n");
        pos += 1;
    }

    return text;
}


} // namespace horizon
