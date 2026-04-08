#include "WaylandClipboardBackend.hpp"
#include <horizon/WaylandSurface.hpp>
#include "MainThreadDataSink.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <vector>
#include <horizon/Logger.hpp>

namespace horizon {

WaylandClipboardBackend::WaylandClipboardBackend(WaylandSurface* surface)
    : m_surface(surface) {
    static const struct wl_data_device_listener device_listener = {
        data_device_handle_data_offer,
        data_device_handle_enter,
        data_device_handle_leave,
        data_device_handle_motion,
        data_device_handle_drop,
        data_device_handle_selection
    };
    if (m_surface->data_device()) {
        wl_data_device_add_listener(m_surface->data_device(), &device_listener, this);
    }
}

WaylandClipboardBackend::~WaylandClipboardBackend() {
    cleanup_source();
    cleanup_offer();
}

void WaylandClipboardBackend::set_provider(ClipboardProvider* provider, const std::vector<std::string>& mime_types) {
    cleanup_source();
    m_local_provider = provider;
    m_owned_provider.reset(); // Clear any static data

    struct wl_data_device_manager* manager = (struct wl_data_device_manager*)m_surface->data_device_manager();

    m_source = wl_data_device_manager_create_data_source(manager);
    
    static const struct wl_data_source_listener source_listener = {
        data_source_handle_target,
        data_source_handle_send,
        data_source_handle_cancelled,
        nullptr, // dnd_drop_performed
        nullptr, // dnd_finished
        nullptr  // action
    };
    wl_data_source_add_listener(m_source, &source_listener, this);

    for (const auto& mime : mime_types) {
        wl_data_source_offer(m_source, mime.c_str());
    }

    struct wl_data_device* device = (struct wl_data_device*)m_surface->data_device();
    wl_data_device_set_selection(device, m_source, m_surface->last_serial());
}

void WaylandClipboardBackend::clear_provider() {
    cleanup_source();
    m_local_provider = nullptr;
    struct wl_data_device* device = (struct wl_data_device*)m_surface->data_device();
    wl_data_device_set_selection(device, nullptr, m_surface->last_serial());
}

void WaylandClipboardBackend::request_data(const std::string& mime, std::shared_ptr<DataSink> sink) {
    // If we have a local provider, bypass Wayland loopback for maximum efficiency and reliability
    if (m_local_provider) {
        m_local_provider->provide_clipboard_data(mime, *sink);
        sink->done();
        return;
    }

    if (!m_current_offer) {
        sink->error();
        return;
    }

    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) == -1) {
        sink->error();
        return;
    }

    wl_data_offer_receive(m_current_offer, mime.c_str(), fds[1]);
    close(fds[1]);
    
    // CRITICAL: Flush the display to ensure the receive request is sent to the compositor immediately.
    // Without this, the compositor won't start writing to the pipe, and our poll will time out.
    wl_display_flush(m_surface->display());

    // Fast-track: try to read immediately
    struct pollfd pfd;
    pfd.fd = fds[0];
    pfd.events = POLLIN;

    std::vector<uint8_t> total_data;
    std::vector<uint8_t> buffer(4096);

    while (true) {
        int ret = poll(&pfd, 1, 200); // 200ms timeout per chunk
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) break; // Timeout

        ssize_t n = read(fds[0], buffer.data(), buffer.size());
        if (n > 0) {
            sink->write(std::vector<uint8_t>(buffer.begin(), buffer.begin() + n));
        } else if (n == 0) {
            break; // EOF
        } else if (errno != EAGAIN && errno != EINTR) {
            sink->error();
            close(fds[0]);
            return;
        }
    }

    sink->done();
    close(fds[0]);
}

SelectionState WaylandClipboardBackend::get_state() const {
    if (m_local_provider) return SelectionState::LOCAL_OWNER;
    if (m_current_offer) return SelectionState::REMOTE_OFFER;
    return SelectionState::IDLE;
}

void WaylandClipboardBackend::on_widget_destroyed(Widget* widget) {
    if (m_local_provider == (ClipboardProvider*)widget) {
        clear_provider();
    }
}

namespace {
    class StaticClipboardProvider : public ClipboardProvider {
        ClipboardData m_data;
    public:
        StaticClipboardProvider(const ClipboardData& data) : m_data(data) {}
        void provide_clipboard_data(const std::string& mime, DataSink& sink) override {
            auto bytes = m_data.get(mime);
            if (!bytes.empty()) {
                sink.write(bytes);
                sink.done();
            } else {
                sink.error();
            }
        }
        std::vector<std::string> provided_mime_types() const override {
            return m_data.mime_types();
        }
    };
}

void WaylandClipboardBackend::set(const ClipboardData& data) {
    m_owned_provider = std::make_unique<StaticClipboardProvider>(data);
    set_provider(m_owned_provider.get(), m_owned_provider->provided_mime_types());
}

void WaylandClipboardBackend::cleanup_source() {
    if (m_source) {
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }
}

void WaylandClipboardBackend::cleanup_offer() {
    if (m_current_offer) {
        wl_data_offer_destroy(m_current_offer);
        m_current_offer = nullptr;
    }
    m_offered_mime_types.clear();
}

// Global callbacks implementation
void WaylandClipboardBackend::data_source_handle_target(void *data, struct wl_data_source *source, const char *mime_type) {}

void WaylandClipboardBackend::data_source_handle_send(void *data, struct wl_data_source *source, const char *mime_type, int32_t fd) {
    auto self = static_cast<WaylandClipboardBackend*>(data);
    if (self->m_local_provider && mime_type) {
        // Simple synchronous write for the data source.
        // In a more complex scenario, we could use a thread, but for clipboard text this is usually fine.
        struct FDDataSink : public DataSink {
            int fd;
            FDDataSink(int f) : fd(f) {}
            void write(const std::vector<uint8_t>& data) override {
                ::write(fd, data.data(), data.size());
            }
            void done() override {}
            void error() override {}
        };
        
        FDDataSink sink(fd);
        self->m_local_provider->provide_clipboard_data(mime_type, sink);
    }
    close(fd);
}

void WaylandClipboardBackend::data_source_handle_cancelled(void *data, struct wl_data_source *source) {
    auto self = static_cast<WaylandClipboardBackend*>(data);
    self->cleanup_source();
    self->m_local_provider = nullptr;
}

void WaylandClipboardBackend::data_device_handle_data_offer(void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    auto self = static_cast<WaylandClipboardBackend*>(data);
    static const struct wl_data_offer_listener offer_listener = {
        data_offer_handle_offer,
        nullptr, // source_actions
        nullptr  // action
    };
    wl_data_offer_add_listener(id, &offer_listener, self);
}

void WaylandClipboardBackend::data_device_handle_enter(void *, struct wl_data_device *, uint32_t, struct wl_surface *, wl_fixed_t, wl_fixed_t, struct wl_data_offer *) {}
void WaylandClipboardBackend::data_device_handle_leave(void *, struct wl_data_device *) {}
void WaylandClipboardBackend::data_device_handle_motion(void *, struct wl_data_device *, uint32_t, wl_fixed_t, wl_fixed_t) {}
void WaylandClipboardBackend::data_device_handle_drop(void *, struct wl_data_device *) {}

void WaylandClipboardBackend::data_device_handle_selection(void *data, struct wl_data_device *data_device, struct wl_data_offer *id) {
    auto self = static_cast<WaylandClipboardBackend*>(data);
    self->cleanup_offer();
    self->m_current_offer = id;
    self->m_offer_counter++;
}

void WaylandClipboardBackend::data_offer_handle_offer(void *data, struct wl_data_offer *offer, const char *mime_type) {
    auto self = static_cast<WaylandClipboardBackend*>(data);
    if (mime_type) {
        self->m_offered_mime_types.push_back(mime_type);
    }
}

} // namespace horizon
