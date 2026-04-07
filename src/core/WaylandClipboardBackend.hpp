#pragma once

#include "horizon/Clipboard.hpp"
#include <wayland-client.h>
#include <memory>
#include <optional>

namespace horizon {

class WaylandSurface;

/**
 * @class WaylandClipboardBackend
 * @brief Internal backend for managing Wayland data device and data source.
 */
class WaylandClipboardBackend {
public:
    WaylandClipboardBackend(WaylandSurface* surface);
    ~WaylandClipboardBackend();

    void set(const ClipboardData& data);
    std::optional<ClipboardData> get(const std::vector<std::string>& preferred_mimes = {});



    // Friend callbacks for Wayland protocol
    static void data_source_handle_target(void *data, struct wl_data_source *source, const char *mime_type);
    static void data_source_handle_send(void *data, struct wl_data_source *source, const char *mime_type, int32_t fd);
    static void data_source_handle_cancelled(void *data, struct wl_data_source *source);
    static void data_source_handle_dnd_drop_performed(void *data, struct wl_data_source *source);
    static void data_source_handle_dnd_finished(void *data, struct wl_data_source *source);
    static void data_source_handle_action(void *data, struct wl_data_source *source, uint32_t action);

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
    std::vector<std::string> m_offered_mime_types;
    struct wl_data_offer* m_pending_offer = nullptr;
    std::vector<std::string> m_pending_mime_types;
    std::optional<ClipboardData> m_current_data;
    int m_offer_counter = 0;




    void cleanup_source();
    void cleanup_offer();
    void cleanup_pending_offer();

};

} // namespace horizon
