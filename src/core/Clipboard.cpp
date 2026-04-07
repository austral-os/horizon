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


namespace horizon {

// --- ClipboardData Implementation ---

void ClipboardData::set(const std::string& mime, const std::vector<uint8_t>& data) {
    m_data[mime] = data;
}

bool ClipboardData::has(const std::string& mime) const {
    return m_data.count(mime) > 0;
}

std::vector<uint8_t> ClipboardData::get(const std::string& mime) const {
    auto it = m_data.find(mime);
    if (it != m_data.end()) return it->second;
    return {};
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
        close(fd);
        return;
    }

    const std::vector<uint8_t>* buffer = nullptr;
    if (self->m_current_data->has(mime_type)) {
        static std::vector<uint8_t> temp_buffer;
        temp_buffer = self->m_current_data->get(mime_type);
        buffer = &temp_buffer;
    } else if (self->m_current_data->has("text/plain")) {
        // Fallback to text/plain
        static std::vector<uint8_t> temp_buffer;
        temp_buffer = self->m_current_data->get("text/plain");
        buffer = &temp_buffer;
    }

    if (buffer && !buffer->empty()) {
        const uint8_t* ptr = buffer->data();
        size_t left = buffer->size();
        
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
                        if (pfd.revents & POLLOUT) continue;
                        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) break;
                    } else {
                        break; // Timeout or error
                    }
                } else {
                    break; // Unrecoverable error
                }
            } else {
                break; // Should not happen for pipe
            }
        }
    }
    close(fd);
}

void WaylandClipboardBackend::data_source_handle_cancelled(void *data, struct wl_data_source *) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
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
    
    LOG_INFO << "WaylandClipboardBackend: new pending offer received: " << id;

    
    // Attach the listener immediately to capture advertised MIME types

    wl_data_offer_add_listener(id, &data_offer_listener, self);
}



void WaylandClipboardBackend::data_device_handle_enter(void *, struct wl_data_device *, uint32_t, struct wl_surface *, wl_fixed_t, wl_fixed_t, struct wl_data_offer *) {}
void WaylandClipboardBackend::data_device_handle_leave(void *, struct wl_data_device *) {}
void WaylandClipboardBackend::data_device_handle_motion(void *, struct wl_data_device *, uint32_t, wl_fixed_t, wl_fixed_t) {}
void WaylandClipboardBackend::data_device_handle_drop(void *, struct wl_data_device *) {}

void WaylandClipboardBackend::data_device_handle_selection(void *data, struct wl_data_device *, struct wl_data_offer *id) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    
    if (id == nullptr) {
        self->cleanup_offer();
        return;
    }
    
    // If this is our pending offer being promoted to selection
    if (id == self->m_pending_offer) {
        LOG_INFO << "WaylandClipboardBackend: promoting pending offer to selection: " << id;
        self->cleanup_offer(); // Cleanup previous active selection
        self->m_current_offer = self->m_pending_offer;
        self->m_offered_mime_types = std::move(self->m_pending_mime_types);
        
        self->m_pending_offer = nullptr;
        self->m_pending_mime_types.clear();
    } else if (id != self->m_current_offer) {

        // External selection we didn't see a data_offer for yet? 
        // (Protocol shouldn't allow this, but handle anyway)
        self->cleanup_offer();
        self->m_current_offer = id;
        // In this case m_offered_mime_types might be empty, 
        // but we normally get data_offer first.
    }
}

void WaylandClipboardBackend::data_offer_handle_offer(void *data, struct wl_data_offer *offer, const char *mime_type) {
    auto* self = static_cast<WaylandClipboardBackend*>(data);
    if (offer == self->m_pending_offer) {
        self->m_pending_mime_types.push_back(mime_type);
    } else if (offer == self->m_current_offer) {


        self->m_offered_mime_types.push_back(mime_type);
    }
}


std::optional<ClipboardData> WaylandClipboardBackend::get(const std::vector<std::string>& preferred_mimes) {
    if (!m_current_offer || !m_surface) return std::nullopt;

    auto* display = m_surface->display();
    if (!display) return std::nullopt;

    // Snapshot state to detect re-entrancy and race conditions
    wl_data_offer* offer = m_current_offer;
    int initial_counter = m_offer_counter;

    ClipboardData result;
    const size_t MAX_CLIPBOARD_SIZE = 1 * 1024 * 1024; // 1MB limit

    // Determine which MIME types to fetch
    const std::vector<std::string>& target_mimes = preferred_mimes.empty() ? m_offered_mime_types : preferred_mimes;

    LOG_INFO << "WaylandClipboardBackend::get: searching through " << target_mimes.size() << " mime types";

    for (const auto& mime : target_mimes) {
        // Early exit if selection was replaced before we even started this format
        if (m_offer_counter != initial_counter) {
            LOG_INFO << "WaylandClipboardBackend: selection changed during get(), aborting";
            return std::nullopt;
        }

        // Check if the current offer actually has this mime type
        bool available = false;
        if (preferred_mimes.empty()) {
            available = true; // Iterating m_offered_mime_types
        } else {
            for (const auto& offered : m_offered_mime_types) {
                if (offered == mime) { available = true; break; }
            }
        }
        if (!available) continue;

        LOG_INFO << "WaylandClipboardBackend::get: requesting " << mime;
        int pipefd[2];
        if (pipe(pipefd) == -1) continue;

        // Set non-blocking read
        fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);

        wl_data_offer_receive(offer, mime.c_str(), pipefd[1]);
        close(pipefd[1]); // Close write end in our process

        // Double check counter after receive call
        if (m_offer_counter != initial_counter) {
            LOG_INFO << "WaylandClipboardBackend: selection changed during receive, aborting";
            close(pipefd[0]);
            return std::nullopt;
        }

        wl_display_flush(display);
        
        // Dispatch once to let the request reach the compositor
        wl_display_dispatch(display);

        if (m_offer_counter != initial_counter) {
            LOG_INFO << "WaylandClipboardBackend: selection changed during dispatch, aborting";
            close(pipefd[0]);
            return std::nullopt;
        }

        std::vector<uint8_t> data;
        uint8_t buffer[4096];
        bool limit_exceeded = false;
        
        // We allow up to 500ms for the data to start arriving
        struct pollfd pfd;
        pfd.fd = pipefd[0];
        pfd.events = POLLIN;

        while (true) {
            // Check if selection changed mid-read
            if (m_offer_counter != initial_counter) break;

            int poll_ret = poll(&pfd, 1, 500); // 500ms timeout
            if (poll_ret <= 0) {
                if (poll_ret == -1 && errno == EINTR) continue;
                LOG_INFO << "WaylandClipboardBackend: read timeout or error for " << mime;
                break; 
            }

            ssize_t n = read(pipefd[0], buffer, sizeof(buffer));
            if (n > 0) {
                if (data.size() + n > MAX_CLIPBOARD_SIZE) {
                    size_t allowed = MAX_CLIPBOARD_SIZE - data.size();
                    data.insert(data.end(), buffer, buffer + allowed);
                    limit_exceeded = true;
                    break;
                }
                data.insert(data.end(), buffer, buffer + n);
            } else if (n == 0) {
                break; // EOF
            } else if (n == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // This shouldn't happen right after poll, but for safety:
                    continue;
                }
                if (errno == EINTR) continue;
                break; // Error
            }
        }

        close(pipefd[0]);

        
        // Re-check counter one last time for this mime type
        if (m_offer_counter != initial_counter) return std::nullopt;

        if (!data.empty()) {
            result.set(mime, data);
            
            // If we satisfy the preferred list request, stop early
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

    return std::string(bytes.begin(), bytes.end());
}



} // namespace horizon
