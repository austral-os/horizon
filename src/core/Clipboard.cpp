#include "horizon/Clipboard.hpp"
#include "WaylandClipboardBackend.hpp"
#include "horizon/WaylandSurface.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/Logger.hpp"
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <errno.h>

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
    self->cleanup_offer();
    self->m_current_offer = id;
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
    // id is the new selection. If it's different from our current offer, it means selection changed externally.
    if (id != self->m_current_offer) {
        self->cleanup_offer();
        self->m_current_offer = id;
    }
}

} // namespace horizon
