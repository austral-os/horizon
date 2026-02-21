#pragma once

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_surface;
struct xdg_wm_base;
struct wl_buffer;

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

        struct xdg_wm_base *xdg_wm_base() const;
        void *data() const;
        struct wl_surface *surface() const;
        struct wl_buffer *buffer() const;

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
    };
}; // namespace horizon