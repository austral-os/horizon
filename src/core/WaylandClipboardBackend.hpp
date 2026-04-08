#pragma once

#include "horizon/Clipboard.hpp"
#include "horizon/ClipboardBackend.hpp"
#include <wayland-client.h>
#include <memory>
#include <optional>
#include <vector>
#include <string>

namespace horizon {

class WaylandSurface;

/**
 * @class WaylandClipboardBackend
 * @brief Wayland-specific implementation of the clipboard backend.
 */
class WaylandClipboardBackend : public ClipboardBackend {
public:
    WaylandClipboardBackend(WaylandSurface* surface);
    ~WaylandClipboardBackend();

    // ClipboardBackend interface implementation
    void set_provider(ClipboardProvider* provider, const std::vector<std::string>& mime_types) override;
    void clear_provider() override;
    std::vector<std::string> get_mime_types() const override { return m_offered_mime_types; }
    void request_data(const std::string& mime, std::shared_ptr<DataSink> sink) override;
    SelectionState get_state() const override;
    uint64_t get_current_generation() const override { return m_offer_counter; }
    void on_widget_destroyed(Widget* widget) override;

    // Legacy support (to be removed once fully migrated)
    void set(const ClipboardData& data);

    // Wayland protocol callbacks
    static void data_source_handle_target(void *data, struct wl_data_source *source, const char *mime_type);
    static void data_source_handle_send(void *data, struct wl_data_source *source, const char *mime_type, int32_t fd);
    static void data_source_handle_cancelled(void *data, struct wl_data_source *source);
    
    static void data_device_handle_data_offer(void *data, struct wl_data_device *data_device, struct wl_data_offer *id);
    static void data_device_handle_enter(void *data, struct wl_data_device *data_device, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id);
    static void data_device_handle_leave(void *data, struct wl_data_device *data_device);
    static void data_device_handle_motion(void *data, struct wl_data_device *data_device, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    static void data_device_handle_drop(void *data, struct wl_data_device *data_device);
    static void data_device_handle_selection(void *data, struct wl_data_device *data_device, struct wl_data_offer *id);
    static void data_offer_handle_offer(void *data, struct wl_data_offer *offer, const char *mime_type);

private:
    WaylandSurface* m_surface;
    struct wl_data_source* m_source = nullptr;
    struct wl_data_offer* m_current_offer = nullptr;
    
    ClipboardProvider* m_local_provider = nullptr;
    std::unique_ptr<ClipboardProvider> m_owned_provider;
    std::vector<std::string> m_offered_mime_types;
    uint64_t m_offer_counter = 0;

    void cleanup_source();
    void cleanup_offer();
};

} // namespace horizon
