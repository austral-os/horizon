#include <cstring>
#include <fcntl.h>
#include <horizon/WaylandSurface.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include <wayland-client-core.h>
#include <wayland-client.h>
#include <wayland-cursor.h>

namespace horizon
{

    /**
     * @brief Handler for seat capability changes (pointer, keyboard).
     */
    static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps);

    /**
     * @brief Handler for seat name changes.
     */
    static void seat_handle_name(void *data, wl_seat *seat, const char *name);

    /**
     * @brief Handler for pointer entering a surface.
     */
    static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);

    /**
     * @brief Handler for pointer leaving a surface.
     */
    static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface);

    /**
     * @brief Handler for pointer motion events.
     */
    static void pointer_handle_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t sx,
                                      wl_fixed_t sy);

    /**
     * @brief Handler for pointer button events.
     */
    static void pointer_handle_button(void *data, wl_pointer *pointer, uint32_t serial,
                                      uint32_t time, uint32_t button, uint32_t state);

    /**
     * @brief Handler for pointer axis/scroll events.
     */
    static void pointer_handle_axis(void *data, wl_pointer *pointer, uint32_t time, uint32_t axis,
                                    wl_fixed_t value);

    /**
     * @brief Handler for keyboard keymap updates.
     */
    static void keyboard_handle_keymap(void *data, wl_keyboard *keyboard, uint32_t format,
                                       int32_t fd, uint32_t size);

    /**
     * @brief Handler for keyboard entering a surface (gaining focus).
     */
    static void keyboard_handle_enter(void *data, wl_keyboard *keyboard, uint32_t serial,
                                      struct wl_surface *surface, struct wl_array *keys);

    /**
     * @brief Handler for keyboard leaving a surface (losing focus).
     */
    static void keyboard_handle_leave(void *data, wl_keyboard *keyboard, uint32_t serial,
                                      struct wl_surface *surface);

    /**
     * @brief Handler for keyboard key press/release events.
     */
    static void keyboard_handle_key(void *data, wl_keyboard *keyboard, uint32_t serial,
                                    uint32_t time, uint32_t key, uint32_t state);

    /**
     * @brief Handler for keyboard modifier changes (Shift, Ctrl, etc.).
     */
    static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial,
                                          uint32_t mods_depressed, uint32_t mods_latched,
                                          uint32_t mods_locked, uint32_t group);

    /**
     * @brief Handler for keyboard repeat settings.
     */
    static void keyboard_handle_repeat_info(void *data, wl_keyboard *keyboard, int32_t rate,
                                            int32_t delay);

    /**
     * @brief Dispatch table for pointer events.
     */
    static const wl_pointer_listener g_pointer_listener = {
        pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button,
        pointer_handle_axis};

    /**
     * @brief Dispatch table for keyboard events.
     */
    static const wl_keyboard_listener g_keyboard_listener = {
        keyboard_handle_keymap, keyboard_handle_enter,     keyboard_handle_leave,
        keyboard_handle_key,    keyboard_handle_modifiers, keyboard_handle_repeat_info};

    /**
     * @brief Dispatch table for seat events.
     */
    static const wl_seat_listener g_seat_listener = {seat_handle_capabilities, seat_handle_name};

    static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);

        if (caps & WL_SEAT_CAPABILITY_POINTER)
        {
            self->set_wl_pointer(static_cast<wl_pointer *>(wl_seat_get_pointer(seat)));
            wl_pointer_add_listener(self->pointer(), &g_pointer_listener, data);
        }

        if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
        {
            self->set_wl_keyboard(static_cast<wl_keyboard *>(wl_seat_get_keyboard(seat)));
            wl_keyboard_add_listener(self->keyboard(), &g_keyboard_listener, data);
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
        WaylandSurface *self = static_cast<WaylandSurface *>(data);
        self->set_last_serial(serial);

        if (self->listener())
        {
            PointerEvent ev;
            ev.type = (state == WL_POINTER_BUTTON_STATE_PRESSED) ? PointerEvent::Type::Press
                                                                 : PointerEvent::Type::Release;
            ev.x = self->pointer_x();
            ev.y = self->pointer_y();
            ev.button = button;
            ev.serial = serial;
            self->listener()->on_pointer_event(ev);
        }
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
        self->set_last_serial(serial);

        if (self->listener())
        {
            PointerEvent ev;
            ev.type = PointerEvent::Type::Enter;
            ev.x = self->pointer_x();
            ev.y = self->pointer_y();
            ev.serial = serial;
            self->listener()->on_pointer_event(ev);
        }
    }

    static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);
        self->set_last_serial(serial);

        if (self->listener())
        {
            PointerEvent ev;
            ev.type = PointerEvent::Type::Leave;
            ev.serial = serial;
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

    /**
     * @brief Global registry handler. Binds core Wayland interfaces.
     */
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
            wl_seat_add_listener(ws->seat(), &g_seat_listener, ws);
        }
    }

    /**
     * @brief Handler for removed global objects.
     */
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

    void WaylandSurface::set_wl_keyboard(struct wl_keyboard *keyboard)
    {
        m_keyboard = keyboard;
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
        // Establish connection to the Wayland server.
        m_display = wl_display_connect(nullptr);
        if (!m_display)
        {
            throw std::runtime_error("No se pudo conectar al servidor Wayland.");
        }

        // Get the registry to find available global objects.
        m_registry = wl_display_get_registry(m_display);
        if (!m_registry)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
            throw std::runtime_error("No se pudo obtener el registro.");
        }

        // Add registry listener.
        static const wl_registry_listener listener = {registry_global, registry_global_remove};
        wl_registry_add_listener(m_registry, &listener, this);

        // Initial roundtrip to ensure globals are bound and initialized.
        wl_display_roundtrip(m_display);

        // 1. Create the fundamental wl_surface.
        m_surface = wl_compositor_create_surface(m_compositor);

        // 2. Setup XDG Surface (the window "shell").
        xdg_surface *xdg_surf = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
        static const xdg_surface_listener xdg_surf_ptr = {
            .configure = [](void *data, xdg_surface *xdg_s, uint32_t serial)
            { xdg_surface_ack_configure(xdg_s, serial); }};
        xdg_surface_add_listener(xdg_surf, &xdg_surf_ptr, nullptr);

        // 3. Setup Toplevel (the actual window).
        m_xdg_toplevel = xdg_surface_get_toplevel(xdg_surf);
        xdg_toplevel_set_title(m_xdg_toplevel, "Cairo Wayland Corrected");

        // Listener for toplevel events (configure, close)
        static const xdg_toplevel_listener toplevel_list = {
            .configure =
                [](void *data, xdg_toplevel *, int32_t width, int32_t height,
                   struct wl_array *states)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                uint32_t *state;
                bool maximized = false;
                for (state = static_cast<uint32_t *>(states->data);
                     reinterpret_cast<const char *>(state) <
                     (static_cast<const char *>(states->data) + states->size);
                     state++)
                {
                    if (*state == XDG_TOPLEVEL_STATE_MAXIMIZED)
                        maximized = true;
                }
                self->m_is_maximized = maximized;
                if (width > 0 && height > 0)
                {
                    self->resize_buffer(width, height);
                    if (self->m_listener)
                    {
                        self->m_listener->on_resize(width, height);
                    }
                }
            },
            .close =
                [](void *data, xdg_toplevel *)
            {
                // Not implemented here, handled by application loop usually
            }};
        xdg_toplevel_add_listener(m_xdg_toplevel, &toplevel_list, this);

        // IMPORTANT: Commit the surface so the compositor can start processing it.
        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        // Initialize cursor resources
        if (m_shm)
        {
            m_cursor_theme = wl_cursor_theme_load(nullptr, 24, m_shm);
        }
        if (m_compositor)
        {
            m_cursor_surface = wl_compositor_create_surface(m_compositor);
        }

        // 4. Create a Shared Memory (SHM) buffer for rendering.
        int stride = m_width * 4;
        int size = stride * m_height;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        if (fd < 0)
        {
            throw std::runtime_error("No se pudo crear el descriptor de archivo para SHM.");
        }
        ftruncate(fd, size);
        m_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        wl_shm_pool *pool = wl_shm_create_pool(m_shm, fd, size);
        m_buffer =
            wl_shm_pool_create_buffer(pool, 0, m_width, m_height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);
    }

    void WaylandSurface::resize_buffer(int width, int height)
    {
        if (width == m_width && height == m_height)
            return;

        // 1. Cleanup old resources
        if (m_buffer)
        {
            wl_buffer_destroy(m_buffer);
        }
        if (m_data)
        {
            munmap(m_data, m_width * m_height * 4);
        }

        m_width = width;
        m_height = height;

        // 2. Reallocate
        int stride = m_width * 4;
        int size = stride * m_height;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        if (fd < 0)
        {
            throw std::runtime_error(
                "No se pudo crear el descriptor de archivo para SHM en resize.");
        }
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
        if (m_keyboard)
        {
            wl_keyboard_destroy(m_keyboard);
            m_keyboard = nullptr;
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
        if (m_xdg_toplevel)
        {
            xdg_toplevel_destroy(m_xdg_toplevel);
            m_xdg_toplevel = nullptr;
        }
        if (m_display)
        {
            if (m_cursor_theme)
            {
                wl_cursor_theme_destroy(m_cursor_theme);
                m_cursor_theme = nullptr;
            }
            if (m_cursor_surface)
            {
                wl_surface_destroy(m_cursor_surface);
                m_cursor_surface = nullptr;
            }
            wl_display_disconnect(m_display);
            m_display = nullptr;
        }
    }

    void WaylandSurface::request_move(uint32_t serial)
    {
        if (m_xdg_toplevel && m_seat)
        {
            xdg_toplevel_move(m_xdg_toplevel, m_seat, serial);
        }
    }

    void WaylandSurface::request_maximize()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_maximized(m_xdg_toplevel);
        }
    }

    void WaylandSurface::request_minimize()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_minimized(m_xdg_toplevel);
        }
    }

    void WaylandSurface::request_restore()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_unset_maximized(m_xdg_toplevel);
        }
    }

    void WaylandSurface::set_cursor(CursorType type)
    {
        if (!m_pointer || !m_cursor_theme || !m_cursor_surface)
            return;

        std::vector<const char *> names;
        switch (type)
        {
        case CursorType::Default:
            names = {"left_ptr", "default", "arrow"};
            break;
        case CursorType::Pointer:
            names = {"hand1", "hand2", "pointer", "pointing_hand"};
            break;
        case CursorType::Text:
            names = {"xterm", "text", "ibeam"};
            break;
        case CursorType::Move:
            names = {"fleur", "move", "grabbing", "alias"};
            break;
        case CursorType::Wait:
            names = {"watch", "wait", "left_ptr_watch"};
            break;
        case CursorType::Help:
            names = {"question_arrow", "help", "whats_this"};
            break;
        default:
            names = {"left_ptr"};
            break;
        }

        struct wl_cursor *cursor = nullptr;
        for (const char *name : names)
        {
            cursor = wl_cursor_theme_get_cursor(m_cursor_theme, name);
            if (cursor)
                break;
        }

        if (!cursor)
            return;

        struct wl_cursor_image *image = cursor->images[0];
        struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);
        if (!buffer)
            return;

        wl_pointer_set_cursor(m_pointer, m_last_serial, m_cursor_surface, image->hotspot_x,
                              image->hotspot_y);
        wl_surface_attach(m_cursor_surface, buffer, 0, 0);
        wl_surface_damage(m_cursor_surface, 0, 0, image->width, image->height);
        wl_surface_commit(m_cursor_surface);

        m_current_cursor_type = type;
    }

    bool WaylandSurface::is_maximized() const
    {
        return m_is_maximized;
    }

    void WaylandSurface::set_last_serial(uint32_t serial)
    {
        m_last_serial = serial;
    }

    uint32_t WaylandSurface::last_serial() const
    {
        return m_last_serial;
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

    struct wl_keyboard *WaylandSurface::keyboard() const
    {
        return m_keyboard;
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