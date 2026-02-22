#pragma once

#include "horizon/WaylandEventListener.hpp"
#include <cstdint>

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_surface;
struct xdg_wm_base;
struct wl_buffer;
struct wl_seat;
struct wl_pointer;

namespace horizon
{
    class WaylandSurface
    {
    public:
        explicit WaylandSurface(int w, int h);
        ~WaylandSurface();

        void set_wl_compositor(struct wl_compositor *compositor);
        void set_wl_shm(struct wl_shm *shm);
        void set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base);
        void set_wl_seat(struct wl_seat *seat);
        void set_wl_pointer(struct wl_pointer *pointer);

        void set_event_listener(WaylandEventListener *listener);

        void set_pointer_x(double x);
        void set_pointer_y(double y);

        struct wl_pointer *pointer() const;
        struct wl_seat *seat() const;
        struct xdg_wm_base *xdg_wm_base() const;
        void *data() const;
        struct wl_surface *surface() const;
        struct wl_buffer *buffer() const;
        struct wl_display *display() const;
        WaylandEventListener *listener() const;
        double pointer_x() const;
        double pointer_y() const;

        int width() const;
        int height() const;

        void init();
        void free();

    private:
        int m_width;
        int m_height;
        struct wl_display *m_display = nullptr;
        struct wl_registry *m_registry = nullptr;
        struct wl_compositor *m_compositor = nullptr;
        struct wl_shm *m_shm = nullptr;
        struct xdg_wm_base *m_xdg_wm_base = nullptr;
        void *m_data;
        struct wl_surface *m_surface;
        struct wl_buffer *m_buffer;

        WaylandEventListener *m_listener = nullptr;

        struct wl_seat *m_seat = nullptr;
        struct wl_pointer *m_pointer = nullptr;

        uint32_t m_last_serial = 0;
        double m_pointer_x = 0;
        double m_pointer_y = 0;
    };
}; // namespace horizon