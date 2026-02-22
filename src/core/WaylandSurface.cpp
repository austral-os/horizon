#include <cstring>
#include <horizon/WaylandSurface.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client.h>

namespace horizon
{

    static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps);
    static void seat_handle_name(void *data, wl_seat *seat, const char *name);
    static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface);
    static void pointer_handle_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t sx,
                                      wl_fixed_t sy);
    static void pointer_handle_button(void *data, wl_pointer *pointer, uint32_t serial,
                                      uint32_t time, uint32_t button, uint32_t state);
    static void pointer_handle_axis(void *data, wl_pointer *pointer, uint32_t time, uint32_t axis,
                                    wl_fixed_t value);

    static void keyboard_handle_keymap(void *data, wl_keyboard *keyboard, uint32_t format,
                                       int32_t fd, uint32_t size);
    static void keyboard_handle_enter(void *data, wl_keyboard *keyboard, uint32_t serial,
                                      struct wl_surface *surface, struct wl_array *keys);
    static void keyboard_handle_leave(void *data, wl_keyboard *keyboard, uint32_t serial,
                                      struct wl_surface *surface);
    static void keyboard_handle_key(void *data, wl_keyboard *keyboard, uint32_t serial,
                                    uint32_t time, uint32_t key, uint32_t state);
    static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial,
                                          uint32_t mods_depressed, uint32_t mods_latched,
                                          uint32_t mods_locked, uint32_t group);
    static void keyboard_handle_repeat_info(void *data, wl_keyboard *keyboard, int32_t rate,
                                            int32_t delay);

    static const wl_pointer_listener g_pointer_listener = {
        pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button,
        pointer_handle_axis};

    static const wl_keyboard_listener g_keyboard_listener = {
        keyboard_handle_keymap, keyboard_handle_enter,     keyboard_handle_leave,
        keyboard_handle_key,    keyboard_handle_modifiers, keyboard_handle_repeat_info};

    static const wl_seat_listener g_seat_listener = {seat_handle_capabilities, seat_handle_name};

    static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);

        if (caps & WL_SEAT_CAPABILITY_POINTER)
        {
            self->set_wl_pointer(static_cast<wl_pointer *>(wl_seat_get_pointer(seat)));

            wl_pointer_add_listener(self->pointer(), &g_pointer_listener, data);
        }
    }

    static void pointer_handle_motion(void *data, wl_pointer *, uint32_t, wl_fixed_t sx,
                                      wl_fixed_t sy)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);

        self->set_pointer_x(wl_fixed_to_double(sx));
        self->set_pointer_y(wl_fixed_to_double(sy));

        if (self->listener())
        {
            PointerEvent ev;
            ev.type = PointerEvent::Type::Move;
            ev.x = self->pointer_x();
            ev.y = self->pointer_y();
            self->listener()->on_pointer_event(ev);
        }
    }

    static void pointer_handle_button(void *data, wl_pointer *pointer, uint32_t serial,
                                      uint32_t time, uint32_t button, uint32_t state)
    {
    }

    static void pointer_handle_axis(void *data, wl_pointer *pointer, uint32_t time, uint32_t axis,
                                    wl_fixed_t value)
    {
    }

    static void seat_handle_name(void *data, wl_seat *seat, const char *name) {}

    static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);

        self->set_pointer_x(wl_fixed_to_double(sx));
        self->set_pointer_y(wl_fixed_to_double(sy));

        if (self->listener())
        {
            PointerEvent ev;
            ev.type = PointerEvent::Type::Enter;
            ev.x = self->pointer_x();
            ev.y = self->pointer_y();
            self->listener()->on_pointer_event(ev);
        }
    }

    static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);

        if (self->listener())
        {
            PointerEvent ev;
            ev.type = PointerEvent::Type::Leave;
            self->listener()->on_pointer_event(ev);
        }
    }

    static void keyboard_handle_keymap(void *data, wl_keyboard *keyboard, uint32_t format,
                                       int32_t fd, uint32_t size)
    {
    }

    static void keyboard_handle_enter(void *data, wl_keyboard *keyboard, uint32_t serial,
                                      struct wl_surface *surface, struct wl_array *keys)
    {
    }

    static void keyboard_handle_leave(void *data, wl_keyboard *keyboard, uint32_t serial,
                                      struct wl_surface *surface)
    {
    }

    static void keyboard_handle_key(void *data, wl_keyboard *keyboard, uint32_t serial,
                                    uint32_t time, uint32_t key, uint32_t state)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);

        if (!self->listener())
            return;

        KeyEvent ev;
        ev.type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? KeyEvent::Type::Press
                                                           : KeyEvent::Type::Release;
        ev.key = key;

        self->listener()->on_key_event(ev);
    }

    static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial,
                                          uint32_t mods_depressed, uint32_t mods_latched,
                                          uint32_t mods_locked, uint32_t group)
    {
    }

    static void keyboard_handle_repeat_info(void *data, wl_keyboard *keyboard, int32_t rate,
                                            int32_t delay)
    {
    }

    static void registry_global(void *data, wl_registry *registry, uint32_t id,
                                const char *interface, uint32_t version)
    {
        WaylandSurface *ws = static_cast<WaylandSurface *>(data);

        if (strcmp(interface, "wl_compositor") == 0)
        {
            ws->set_wl_compositor(static_cast<wl_compositor *>(
                wl_registry_bind(registry, id, &wl_compositor_interface, 4)));
        }
        else if (strcmp(interface, "wl_shm") == 0)
        {
            ws->set_wl_shm(
                static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, 1)));
        }
        else if (std::strcmp(interface, "xdg_wm_base") == 0)
        {
            ws->set_xdg_wm_base(static_cast<xdg_wm_base *>(
                wl_registry_bind(registry, id, &xdg_wm_base_interface, 1)));
            // Listener para responder al PING del servidor
            static const xdg_wm_base_listener wm_list = {
                .ping = [](void *, xdg_wm_base *wm, uint32_t ser) { xdg_wm_base_pong(wm, ser); }};
            xdg_wm_base_add_listener(ws->xdg_wm_base(), &wm_list, nullptr);
        }
        else if (strcmp(interface, "wl_seat") == 0)
        {
            ws->set_wl_seat(
                static_cast<wl_seat *>(wl_registry_bind(registry, id, &wl_seat_interface, 1)));
        }
        else if (strcmp(interface, "wl_pointer") == 0)
        {
            ws->set_wl_pointer(static_cast<wl_pointer *>(
                wl_registry_bind(registry, id, &wl_pointer_interface, 1)));
        }
    }

    static void registry_global_remove(void *data, wl_registry *registry, uint32_t id)
    {
        // not implemented
    }

    WaylandSurface::WaylandSurface(int w, int h) : m_width(w), m_height(h) {}

    int WaylandSurface::width() const
    {
        return m_width;
    }
    int WaylandSurface::height() const
    {
        return m_height;
    }

    WaylandSurface::~WaylandSurface()
    {
        free();
    }

    void WaylandSurface::set_wl_seat(struct wl_seat *seat)
    {
        m_seat = seat;
    }

    void WaylandSurface::set_wl_pointer(struct wl_pointer *pointer)
    {
        m_pointer = pointer;
    }

    void WaylandSurface::set_wl_compositor(struct wl_compositor *compositor)
    {
        m_compositor = compositor;
    }

    void WaylandSurface::set_wl_shm(struct wl_shm *shm)
    {
        m_shm = shm;
    }

    void WaylandSurface::set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base)
    {
        m_xdg_wm_base = xdg_wm_base;
    }

    void WaylandSurface::init()
    {

        m_display = wl_display_connect(nullptr);
        if (!m_display)
        {
            throw std::runtime_error("No se pudo conectar al servidor Wayland.");
        }

        m_registry = wl_display_get_registry(m_display);
        if (!m_registry)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
            throw std::runtime_error("No se pudo obtener el registro.");
        }

        // Listener del registry
        static const wl_registry_listener listener = {registry_global, registry_global_remove};
        wl_registry_add_listener(m_registry, &listener, this);

        // Roundtrip inicial para que los globals estén disponibles
        wl_display_roundtrip(m_display);

        // 1. Crear Superficie
        m_surface = wl_compositor_create_surface(m_compositor);

        // 2. Configurar XDG Surface (El "cascarón" de la ventana)
        xdg_surface *xdg_surf = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
        static const xdg_surface_listener xdg_surf_ptr = {
            .configure = [](void *data, xdg_surface *xdg_s, uint32_t serial)
            { xdg_surface_ack_configure(xdg_s, serial); }};
        xdg_surface_add_listener(xdg_surf, &xdg_surf_ptr, nullptr);

        // 3. Configurar Toplevel (La ventana propiamente dicha)
        xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surf);
        xdg_toplevel_set_title(toplevel, "Cairo Wayland Corrected");

        // IMPORTANTE: Primer commit para que el compositor envíe el evento 'configure'
        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        // 4. Crear Buffer SHM
        int stride = m_width * 4;
        int size = stride * m_height;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        ftruncate(fd, size);
        m_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        wl_shm_pool *pool = wl_shm_create_pool(m_shm, fd, size);
        m_buffer =
            wl_shm_pool_create_buffer(pool, 0, m_width, m_height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);
    }

    void WaylandSurface::free()
    {
        if (m_pointer)
        {
            wl_pointer_destroy(m_pointer);
            m_pointer = nullptr;
        }
        if (m_seat)
        {
            wl_seat_destroy(m_seat);
            m_seat = nullptr;
        }

        if (m_data)
        {
            munmap(m_data, m_width * m_height * 4);
            m_data = nullptr;
        }

        if (m_xdg_wm_base)
        {
            xdg_wm_base_destroy(m_xdg_wm_base);
            m_xdg_wm_base = nullptr;
        }
        if (m_compositor)
        {
            wl_compositor_destroy(m_compositor);
            m_compositor = nullptr;
        }
        if (m_shm)
        {
            wl_shm_destroy(m_shm);
            m_shm = nullptr;
        }
        if (m_registry)
        {
            wl_registry_destroy(m_registry);
            m_registry = nullptr;
        }
        if (m_display)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
        }
    }

    void WaylandSurface::set_event_listener(WaylandEventListener *listener)
    {
        m_listener = listener;
    }

    WaylandEventListener *WaylandSurface::listener() const
    {
        return m_listener;
    }

    double WaylandSurface::pointer_x() const
    {
        return m_pointer_x;
    }

    double WaylandSurface::pointer_y() const
    {
        return m_pointer_y;
    }

    void WaylandSurface::set_pointer_x(double x)
    {
        m_pointer_x = x;
    }

    void WaylandSurface::set_pointer_y(double y)
    {
        m_pointer_y = y;
    }

    struct wl_pointer *WaylandSurface::pointer() const
    {
        return m_pointer;
    }

    struct wl_seat *WaylandSurface::seat() const
    {
        return m_seat;
    }

    // Devuelve el puntero a xdg_wm_base
    struct xdg_wm_base *WaylandSurface::xdg_wm_base() const
    {
        return m_xdg_wm_base;
    }

    // Devuelve el puntero al buffer de memoria
    void *WaylandSurface::data() const
    {
        return m_data;
    }

    // Devuelve la superficie Wayland
    struct wl_surface *WaylandSurface::surface() const
    {
        return m_surface;
    }

    // Devuelve el buffer Wayland
    struct wl_buffer *WaylandSurface::buffer() const
    {
        return m_buffer;
    }

    struct wl_display *WaylandSurface::display() const
    {
        return m_display;
    }

} // namespace horizon