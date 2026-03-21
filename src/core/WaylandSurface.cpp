#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <algorithm>
#include <cstring>

#include <fcntl.h>
#include <horizon/Logger.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <horizon/xdg-activation-v1-client-protocol.h>
#include <horizon/xdg-shell-client-protocol.h>
#include <protocols/blur-client-protocol.h>
#include <protocols/ext-background-effect-v1-client-protocol.h>
#include <protocols/ext-foreign-toplevel-list-v1-client-protocol.h>
#include <protocols/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include <wayland-client-core.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

namespace horizon
{
    static WaylandSurface *g_pointer_focus = nullptr;

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

    // Foreign toplevel management listeners
    void foreign_toplevel_handle_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                       const char *title);
    void foreign_toplevel_handle_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                        const char *app_id);
    void foreign_toplevel_handle_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                       struct wl_array *state);
    void foreign_toplevel_handle_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle);
    void foreign_toplevel_handle_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle);

    void foreign_toplevel_handle_output_enter(void *data,
                                              struct zwlr_foreign_toplevel_handle_v1 *handle,
                                              struct wl_output *output);
    void foreign_toplevel_handle_output_leave(void *data,
                                              struct zwlr_foreign_toplevel_handle_v1 *handle,
                                              struct wl_output *output);
    void foreign_toplevel_handle_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                        struct zwlr_foreign_toplevel_handle_v1 *parent);

    static const struct zwlr_foreign_toplevel_handle_v1_listener foreign_toplevel_handle_listener =
        {
            foreign_toplevel_handle_title,        foreign_toplevel_handle_app_id,
            foreign_toplevel_handle_output_enter, foreign_toplevel_handle_output_leave,
            foreign_toplevel_handle_state,        foreign_toplevel_handle_done,
            foreign_toplevel_handle_closed,       foreign_toplevel_handle_parent,
    };

    void foreign_toplevel_manager_finished(void *data,
                                           struct zwlr_foreign_toplevel_manager_v1 *manager);
    void foreign_toplevel_manager_toplevel(void *data,
                                           struct zwlr_foreign_toplevel_manager_v1 *manager,
                                           struct zwlr_foreign_toplevel_handle_v1 *handle);

    static const struct zwlr_foreign_toplevel_manager_v1_listener
        foreign_toplevel_manager_listener = {
            foreign_toplevel_manager_toplevel,
            foreign_toplevel_manager_finished,
    };

    // ext-foreign-toplevel-list-v1 handlers
    void ext_foreign_toplevel_list_handle_toplevel(void *data,
                                                   struct ext_foreign_toplevel_list_v1 *ext_foreign_toplevel_list_v1,
                                                   struct ext_foreign_toplevel_handle_v1 *handle);
    void ext_foreign_toplevel_handle_app_id(void *data,
                                            struct ext_foreign_toplevel_handle_v1 *handle,
                                            const char *app_id);
    void ext_foreign_toplevel_handle_title(void *data,
                                           struct ext_foreign_toplevel_handle_v1 *handle,
                                           const char *title);
    void ext_foreign_toplevel_handle_closed(void *data,
                                            struct ext_foreign_toplevel_handle_v1 *handle);
    void ext_foreign_toplevel_handle_done(void *data,
                                          struct ext_foreign_toplevel_handle_v1 *handle);

    static const struct ext_foreign_toplevel_list_v1_listener ext_foreign_toplevel_list_listener = {
        .toplevel = ext_foreign_toplevel_list_handle_toplevel,
        .finished = [](void *, struct ext_foreign_toplevel_list_v1 *) {},
    };

    static const struct ext_foreign_toplevel_handle_v1_listener ext_foreign_toplevel_handle_listener = {
        .closed = ext_foreign_toplevel_handle_closed,
        .done = ext_foreign_toplevel_handle_done,
        .title = ext_foreign_toplevel_handle_title,
        .app_id = ext_foreign_toplevel_handle_app_id,
        .identifier = [](void *, struct ext_foreign_toplevel_handle_v1 *, const char *) {},
    };

    static void background_effect_manager_handle_capabilities(
        void *data, struct ext_background_effect_manager_v1 *ext_background_effect_manager_v1,
        uint32_t flags)
    {
        // For now we just log it
        LOG_INFO << "[SURFACE] Background effect capabilities: " << flags;
    }

    static const struct ext_background_effect_manager_v1_listener
        background_effect_manager_listener = {
            .capabilities = background_effect_manager_handle_capabilities,
    };

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
        WaylandSurface *self = g_pointer_focus ? g_pointer_focus : static_cast<WaylandSurface *>(data);

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
        WaylandSurface *self = g_pointer_focus ? g_pointer_focus : static_cast<WaylandSurface *>(data);
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
        WaylandSurface *self = static_cast<WaylandSurface *>(data);
        if (self->listener())
        {
            PointerEvent ev;
            ev.type = PointerEvent::Type::Scroll;
            ev.x = self->pointer_x();
            ev.y = self->pointer_y();

            double val = wl_fixed_to_double(value);
            if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
            {
                ev.dy = val;
            }
            else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
            {
                ev.dx = val;
            }

            self->listener()->on_pointer_event(ev);
        }
    }

    static void seat_handle_name(void *data, wl_seat *seat, const char *name) {}

    static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial,
                                     struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
    {
        WaylandSurface *self =
            surface ? static_cast<WaylandSurface *>(wl_surface_get_user_data(surface)) : nullptr;
        if (!self)
            self = static_cast<WaylandSurface *>(data);

        g_pointer_focus = self;

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
        WaylandSurface *self =
            surface ? static_cast<WaylandSurface *>(wl_surface_get_user_data(surface)) : nullptr;
        if (!self)
            self = static_cast<WaylandSurface *>(data);

        if (g_pointer_focus == self)
            g_pointer_focus = nullptr;

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
        WaylandSurface *self = static_cast<WaylandSurface *>(data);
        self->update_xkb_keymap(format, fd, size);
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
        ev.serial = serial;

        self->process_key(key, state, ev);

        self->listener()->on_key_event(ev);
    }

    static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial,
                                          uint32_t mods_depressed, uint32_t mods_latched,
                                          uint32_t mods_locked, uint32_t group)
    {
        WaylandSurface *self = static_cast<WaylandSurface *>(data);
        self->update_xkb_modifiers(mods_depressed, mods_latched, mods_locked, group);

        if (self->listener())
        {
            uint32_t modifiers = 0;
            if (xkb_state_mod_name_is_active(self->xkb_state(), XKB_MOD_NAME_SHIFT,
                                             XKB_STATE_MODS_EFFECTIVE))
                modifiers |= 0x1;
            if (xkb_state_mod_name_is_active(self->xkb_state(), XKB_MOD_NAME_CTRL,
                                             XKB_STATE_MODS_EFFECTIVE))
                modifiers |= 0x2;
            if (xkb_state_mod_name_is_active(self->xkb_state(), XKB_MOD_NAME_ALT,
                                             XKB_STATE_MODS_EFFECTIVE))
                modifiers |= 0x4;
            if (xkb_state_mod_name_is_active(self->xkb_state(), XKB_MOD_NAME_CAPS,
                                             XKB_STATE_MODS_EFFECTIVE))
                modifiers |= 0x8;

            self->listener()->on_modifiers_event(modifiers);
        }
    }

    static void keyboard_handle_repeat_info(void *data, wl_keyboard *keyboard, int32_t rate,
                                            int32_t delay)
    {
    }

    /**
     * @brief Global registry handler. Binds core Wayland interfaces.
     */
    void registry_global(void *data, wl_registry *registry, uint32_t id, const char *interface,
                         uint32_t version)
    {
        WaylandSurface *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] Global: " << interface << " (v" << version << ")";

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
            LOG_INFO << "[SURFACE] Bound xdg_wm_base";
            static const xdg_wm_base_listener wm_list = {
                .ping = [](void *, xdg_wm_base *wm, uint32_t ser) { xdg_wm_base_pong(wm, ser); }};
            xdg_wm_base_add_listener(ws->xdg_wm_base(), &wm_list, nullptr);
        }
        else if (strcmp(interface, "zwlr_layer_shell_v1") == 0)
        {
            ws->set_zwlr_layer_shell(static_cast<zwlr_layer_shell_v1 *>(
                wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1)));
            LOG_INFO << "[SURFACE] Bound zwlr_layer_shell_v1";
        }
        else if (strcmp(interface, "xdg_activation_v1") == 0)
        {
            ws->set_xdg_activation(static_cast<xdg_activation_v1 *>(
                wl_registry_bind(registry, id, &xdg_activation_v1_interface, 1)));
            LOG_INFO << "[SURFACE] Bound xdg_activation_v1";
        }
        else if (strcmp(interface, "zwlr_foreign_toplevel_manager_v1") == 0)
        {
            ws->set_zwlr_foreign_toplevel_manager(static_cast<zwlr_foreign_toplevel_manager_v1 *>(
                wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 3)));
            LOG_INFO << "[SURFACE] Bound zwlr_foreign_toplevel_manager_v1";
            zwlr_foreign_toplevel_manager_v1_add_listener(ws->foreign_toplevel_manager(),
                                                          &foreign_toplevel_manager_listener, ws);
        }
        else if (strcmp(interface, "ext_foreign_toplevel_list_v1") == 0)
        {
            ws->m_ext_foreign_toplevel_list = static_cast<ext_foreign_toplevel_list_v1 *>(
                wl_registry_bind(registry, id, &ext_foreign_toplevel_list_v1_interface, 1));
            LOG_INFO << "[SURFACE] Bound ext_foreign_toplevel_list_v1";
            ext_foreign_toplevel_list_v1_add_listener(ws->m_ext_foreign_toplevel_list,
                                                      &ext_foreign_toplevel_list_listener, ws);
        }
        else if (strcmp(interface, "wl_seat") == 0)
        {
            ws->set_wl_seat(
                static_cast<wl_seat *>(wl_registry_bind(registry, id, &wl_seat_interface, 1)));
            wl_seat_add_listener(ws->seat(), &g_seat_listener, ws);
        }
        else if (strcmp(interface, "wl_output") == 0)
        {
            struct wl_output *output =
                static_cast<wl_output *>(wl_registry_bind(registry, id, &wl_output_interface, 1));
            ws->add_wl_output(output);
        }
        else if (strcmp(interface, ext_background_effect_manager_v1_interface.name) == 0)
        {
            LOG_INFO << "[SURFACE] Binding ext_background_effect_manager_v1";
            ws->set_ext_background_effect_manager(static_cast<ext_background_effect_manager_v1 *>(
                wl_registry_bind(registry, id, &ext_background_effect_manager_v1_interface, 1)));
            LOG_INFO << "[SURFACE] Bound ext_background_effect_manager_v1";
            ext_background_effect_manager_v1_add_listener(ws->background_effect_manager(),
                                                          &background_effect_manager_listener, ws);
        }
        else if (strcmp(interface, org_kde_kwin_blur_manager_interface.name) == 0)
        {
            LOG_INFO << "[SURFACE] Binding org_kde_kwin_blur_manager";
            ws->m_blur_manager = (struct org_kde_kwin_blur_manager *)wl_registry_bind(
                registry, id, &org_kde_kwin_blur_manager_interface, 1);
            LOG_INFO << "[SURFACE] Bound org_kde_kwin_blur_manager";
        }
    }

    /**
     * @brief Handler for removed global objects.
     */
    static void registry_global_remove(void *data, wl_registry *registry, uint32_t id)
    {
        // not implemented
    }

    void WaylandSurface::update_xkb_keymap(uint32_t format, int32_t fd, uint32_t size)
    {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
        {
            close(fd);
            return;
        }

        char *map_str = static_cast<char *>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (map_str == MAP_FAILED)
        {
            close(fd);
            return;
        }

        struct xkb_keymap *keymap = xkb_keymap_new_from_string(
            m_xkb_context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(map_str, size);
        close(fd);

        if (!keymap)
            return;

        struct xkb_state *state = xkb_state_new(keymap);
        if (!state)
        {
            xkb_keymap_unref(keymap);
            return;
        }

        if (m_xkb_state)
            xkb_state_unref(m_xkb_state);
        if (m_xkb_keymap)
            xkb_keymap_unref(m_xkb_keymap);

        m_xkb_keymap = keymap;
        m_xkb_state = state;
    }

    void WaylandSurface::update_xkb_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
                                              uint32_t mods_locked, uint32_t group)
    {
        if (!m_xkb_state)
            return;

        xkb_state_update_mask(m_xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }

    void WaylandSurface::process_key(uint32_t key, uint32_t state, KeyEvent &ev)
    {
        if (!m_xkb_state)
            return;

        xkb_keycode_t keycode = key + 8;
        xkb_keysym_t sym = xkb_state_key_get_one_sym(m_xkb_state, keycode);
        ev.keysym = sym;

        if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
        {
            char buffer[64];
            int size = xkb_state_key_get_utf8(m_xkb_state, keycode, buffer, sizeof(buffer));
            if (size > 0)
            {
                ev.text = std::string(buffer, size);
            }
        }

        // Populate modifiers bitmask from XKB state
        // 0x1: Shift, 0x2: Ctrl, 0x4: Alt, 0x8: CapsLock
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
            ev.modifiers |= 0x1;
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
            ev.modifiers |= 0x2;
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE))
            ev.modifiers |= 0x4;
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_CAPS, XKB_STATE_MODS_EFFECTIVE))
            ev.modifiers |= 0x8;
    }

    WaylandSurface::WaylandSurface(int w, int h) : m_width(w), m_height(h)
    {
        m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

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
        if (m_xkb_state)
            xkb_state_unref(m_xkb_state);
        if (m_xkb_keymap)
            xkb_keymap_unref(m_xkb_keymap);
        if (m_xkb_context)
            xkb_context_unref(m_xkb_context);
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

    void WaylandSurface::set_xdg_activation(struct xdg_activation_v1 *activation)
    {
        m_activation = activation;
    }

    void WaylandSurface::set_wl_shm(struct wl_shm *shm)
    {
        m_shm = shm;
    }

    void WaylandSurface::set_zwlr_foreign_toplevel_manager(
        struct zwlr_foreign_toplevel_manager_v1 *manager)
    {
        m_foreign_toplevel_manager = manager;
    }

    void WaylandSurface::set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base)
    {
        m_xdg_wm_base = xdg_wm_base;
    }

    void WaylandSurface::set_ext_background_effect_manager(
        struct ext_background_effect_manager_v1 *manager)
    {
        m_background_effect_manager = manager;
    }

    void WaylandSurface::set_zwlr_layer_shell(struct zwlr_layer_shell_v1 *layer_shell)
    {
        m_layer_shell = layer_shell;
    }

    struct zwlr_layer_shell_v1 *WaylandSurface::layer_shell() const
    {
        return m_layer_shell;
    }

    void WaylandSurface::init_display()
    {
        if (m_display)
            return;

        m_display = wl_display_connect(nullptr);
        if (!m_display)
            throw std::runtime_error("Failed to connect to Wayland display");

        m_registry = wl_display_get_registry(m_display);
        static const wl_registry_listener listener = {registry_global, registry_global_remove};
        wl_registry_add_listener(m_registry, &listener, this);

        wl_display_roundtrip(m_display);

        init_egl();
    }

    void WaylandSurface::share_connection_from(WaylandSurface *other)
    {
        if (!other)
            return;

        m_display = other->m_display;
        m_registry = other->m_registry;
        m_compositor = other->m_compositor;
        m_shm = other->m_shm;
        m_xdg_wm_base = other->m_xdg_wm_base;
        m_layer_shell = other->m_layer_shell;
        m_seat = other->m_seat;
        m_owns_connection = false;
        m_pointer = other->m_pointer;
        m_keyboard = other->m_keyboard;
        m_activation = other->m_activation;
        m_foreign_toplevel_manager = other->m_foreign_toplevel_manager;
        m_background_effect_manager = other->m_background_effect_manager;
        m_outputs = other->m_outputs;

        m_egl_display = other->m_egl_display;
        m_egl_config = other->m_egl_config;
        m_egl_context = other->m_egl_context;
    }

    void WaylandSurface::init_egl()
    {
        m_egl_display = eglGetDisplay((EGLNativeDisplayType)m_display);
        if (m_egl_display == EGL_NO_DISPLAY)
            throw std::runtime_error("Failed to get EGL display");

        if (!eglInitialize(m_egl_display, nullptr, nullptr))
            throw std::runtime_error("Failed to initialize EGL");

        EGLint attr[] = {EGL_SURFACE_TYPE,
                         EGL_WINDOW_BIT,
                         EGL_RED_SIZE,
                         8,
                         EGL_GREEN_SIZE,
                         8,
                         EGL_BLUE_SIZE,
                         8,
                         EGL_ALPHA_SIZE,
                         8,
                         EGL_RENDERABLE_TYPE,
                         EGL_OPENGL_ES2_BIT,
                         EGL_NONE};

        EGLint num_configs;
        if (!eglChooseConfig(m_egl_display, attr, &m_egl_config, 1, &num_configs) ||
            num_configs < 1)
            throw std::runtime_error("Failed to choose EGL config");

        EGLint ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        m_egl_context = eglCreateContext(m_egl_display, m_egl_config, EGL_NO_CONTEXT, ctx_attr);
        if (m_egl_context == EGL_NO_CONTEXT)
            throw std::runtime_error("Failed to create EGL context");
    }

    void WaylandSurface::setup_xdg_toplevel(const std::string &title, const std::string &app_id)
    {
        m_role = Role::XdgToplevel;
        m_surface = wl_compositor_create_surface(m_compositor);
        wl_surface_set_user_data(m_surface, this);

        m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
        static const xdg_surface_listener xdg_surf_ptr = {
            .configure = [](void *data, xdg_surface *xdg_s, uint32_t serial)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                xdg_surface_ack_configure(xdg_s, serial);
            }};
        xdg_surface_add_listener(m_xdg_surface, &xdg_surf_ptr, this);

        m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
        xdg_toplevel_set_title(m_xdg_toplevel, title.c_str());
        xdg_toplevel_set_app_id(m_xdg_toplevel, app_id.c_str());

        static const xdg_toplevel_listener toplevel_list = {
            .configure =
                [](void *data, xdg_toplevel *, int32_t width, int32_t height,
                   struct wl_array *states)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                uint32_t *state;
                bool maximized = false;
                bool activated = false;
                bool fullscreen = false;
                for (state = static_cast<uint32_t *>(states->data);
                     reinterpret_cast<const char *>(state) <
                     (static_cast<const char *>(states->data) + states->size);
                     state++)
                {
                    if (*state == XDG_TOPLEVEL_STATE_MAXIMIZED)
                        maximized = true;
                    if (*state == XDG_TOPLEVEL_STATE_ACTIVATED)
                        activated = true;
                    if (*state == XDG_TOPLEVEL_STATE_FULLSCREEN)
                        fullscreen = true;
                }
                self->m_is_maximized = maximized;
                self->m_is_fullscreen = fullscreen;

                if (self->m_is_activated != activated)
                {
                    self->m_is_activated = activated;
                    if (self->m_listener)
                    {
                        self->m_listener->on_activated(activated);
                    }
                }

                if (width > 0 && height > 0)
                {
                    self->resize_buffer(width, height);
                    if (self->m_listener)
                        self->m_listener->on_resize(width, height);
                }
            },
            .close =
                [](void *data, xdg_toplevel *)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                if (self->m_listener)
                {
                    self->m_listener->on_close();
                }
            }};
        xdg_toplevel_add_listener(m_xdg_toplevel, &toplevel_list, this);

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        // Cursor setup
        if (m_shm)
            m_cursor_theme = wl_cursor_theme_load(nullptr, 24, m_shm);
        if (m_compositor)
            m_cursor_surface = wl_compositor_create_surface(m_compositor);

        resize_buffer(m_width, m_height);
    }

    void WaylandSurface::setup_layer_surface(uint32_t layer, const std::string &namespace_id)
    {
        m_role = Role::LayerShell;
        m_layer_num = layer;
        m_layer_namespace = namespace_id;
        m_surface = wl_compositor_create_surface(m_compositor);
        wl_surface_set_user_data(m_surface, this);

        if (!m_layer_shell)
            throw std::runtime_error("Compositor does not support wlr-layer-shell");

        m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(m_layer_shell, m_surface, nullptr,
                                                                layer, namespace_id.c_str());

        // CRITICAL: Even for layer surfaces, xdg_popup requires an xdg_surface parent.
        // We create the xdg_surface wrapper here but we don't give it a toplevel role.
        // However, wlroots (labwc, wayfire) strictly forbids multiple roles.
        // We only do this if we are not on a wlroots-based compositor.
        const char *desktop_env = std::getenv("XDG_CURRENT_DESKTOP");
        std::string desktop_str = desktop_env ? desktop_env : "";
        std::transform(desktop_str.begin(), desktop_str.end(), desktop_str.begin(), ::tolower);

        bool is_wlroots = (desktop_str.find("labwc") != std::string::npos ||
                           desktop_str.find("wayfire") != std::string::npos ||
                           desktop_str.find("hzn-labwc") != std::string::npos ||
                           desktop_str.find("hzn-wayfire") != std::string::npos);

        if (!is_wlroots)
        {
            m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
            static const struct xdg_surface_listener layer_xdg_surf_listener = {
                .configure = [](void *data, struct xdg_surface *surf, uint32_t serial)
                {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    self->m_configured = true;
                    xdg_surface_ack_configure(surf, serial);
                }};
            xdg_surface_add_listener(m_xdg_surface, &layer_xdg_surf_listener, this);
        }
        else
        {
            LOG_INFO << "[SURFACE] Skipping xdg_surface wrapper for layer surface on " << desktop_str;
        }

        static const zwlr_layer_surface_v1_listener layer_surface_listener = {
            .configure =
                [](void *data, zwlr_layer_surface_v1 *layer_surface, uint32_t serial,
                   uint32_t width, uint32_t height)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

                if (width > 0 && height > 0)
                {
                    self->resize_buffer(width, height);
                    if (self->m_listener)
                        self->m_listener->on_resize(width, height);
                }
            },
            .closed =
                [](void *data, zwlr_layer_surface_v1 *)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                if (self->m_listener)
                {
                    self->m_listener->on_close();
                }
            }};

        zwlr_layer_surface_v1_add_listener(m_layer_surface, &layer_surface_listener, this);

        // Apply any properties that might have been set before setup
        zwlr_layer_surface_v1_set_anchor(m_layer_surface, m_anchor);
        zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, m_exclusive_zone);
        zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, m_interactivity);
        zwlr_layer_surface_v1_set_size(m_layer_surface, (uint32_t)m_width, (uint32_t)m_height);

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        // Cursor setup for layer surface
        if (m_shm)
            m_cursor_theme = wl_cursor_theme_load(nullptr, 24, m_shm);
        if (m_compositor)
            m_cursor_surface = wl_compositor_create_surface(m_compositor);

        resize_buffer(m_width, m_height);
    }

    void WaylandSurface::setup_xdg_popup(WaylandSurface *parent, int x, int y, int w, int h)
    {
        m_role = Role::XdgPopup;
        share_connection_from(parent);

        m_width = w;
        m_height = h;

        m_surface = wl_compositor_create_surface(m_compositor);
        wl_surface_set_user_data(m_surface, this);
        resize_buffer(w, h); // Initialize m_data and EGL window

        m_xdg_positioner = xdg_wm_base_create_positioner(m_xdg_wm_base);
        xdg_positioner_set_size(m_xdg_positioner, w, h);
        xdg_positioner_set_anchor_rect(m_xdg_positioner, x, y, 1, 1);
        xdg_positioner_set_anchor(m_xdg_positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
        xdg_positioner_set_gravity(m_xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);

        if (parent->m_xdg_surface || parent->m_layer_surface)
        {
            m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);

            static const struct xdg_surface_listener popup_xdg_surf_listener = {
                .configure = [](void *data, struct xdg_surface *surf, uint32_t serial)
                {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    self->m_configured = true;
                    xdg_surface_ack_configure(surf, serial);
                }};
            xdg_surface_add_listener(m_xdg_surface, &popup_xdg_surf_listener, this);

            // If the parent is a layer surface, we MUST pass NULL as the parent to get_popup
            // and then call zwlr_layer_surface_v1_get_popup.
            struct xdg_surface *xdg_parent = parent->m_xdg_surface;
            m_xdg_popup = xdg_surface_get_popup(m_xdg_surface, xdg_parent, m_xdg_positioner);

            if (parent->m_layer_surface)
            {
                zwlr_layer_surface_v1_get_popup(parent->m_layer_surface, m_xdg_popup);
            }
        }
        else
        {
            LOG_ERROR << "[SURFACE] Cannot create popup: Parent surface has no xdg_surface wrapper and is not a layer shell surface.";
        }

        if (m_xdg_popup)
        {
            static const struct xdg_popup_listener popup_listener = {
                .configure = [](void *data, struct xdg_popup *popup, int32_t x, int32_t y, int32_t width, int32_t height)
                {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    if (width > 0 && height > 0)
                    {
                        self->resize_buffer(width, height);
                        if (self->m_listener)
                            self->m_listener->on_resize(width, height);
                    }
                },
                .popup_done = [](void *data, struct xdg_popup *popup)
                {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    if (self->m_listener)
                    {
                        self->m_listener->on_close();
                    }
                },
                .repositioned = [](void *data, struct xdg_popup *popup, uint32_t token)
                {
                    // Handle repositioning if needed
                }};

            xdg_popup_add_listener(m_xdg_popup, &popup_listener, this);

            // Grab pointer for the popup
            if (m_seat)
            {
                xdg_popup_grab(m_xdg_popup, m_seat, parent->last_serial());
            }
        }

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        resize_buffer(m_width, m_height);
    }

    void WaylandSurface::set_layer_anchor(uint32_t anchor)
    {
        m_anchor = anchor;
        if (m_layer_surface)
        {
            zwlr_layer_surface_v1_set_anchor(m_layer_surface, anchor);
        }
    }

    void WaylandSurface::set_layer_exclusive_zone(int32_t zone)
    {
        m_exclusive_zone = zone;
        if (m_layer_surface)
        {
            zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, zone);
        }
    }

    void WaylandSurface::set_layer_keyboard_interactivity(uint32_t interactivity)
    {
        m_interactivity = interactivity;
        if (m_layer_surface)
        {
            zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, interactivity);
        }
    }

    void WaylandSurface::set_layer_size(uint32_t width, uint32_t height)
    {
        m_width = (int)width;
        m_height = (int)height;
        if (m_layer_surface)
        {
            zwlr_layer_surface_v1_set_size(m_layer_surface, width, height);
        }
    }

    void WaylandSurface::set_input_region(int x, int y, int w, int h)
    {
        if (!m_surface || !m_compositor)
            return;

        struct wl_region *region = wl_compositor_create_region(m_compositor);
        wl_region_add(region, x, y, w, h);
        wl_surface_set_input_region(m_surface, region);
        wl_region_destroy(region);
        wl_surface_commit(m_surface);
    }

    void WaylandSurface::clear_input_region()
    {
        if (!m_surface || !m_compositor)
            return;

        // Setting an empty region makes it click-through
        struct wl_region *region = wl_compositor_create_region(m_compositor);
        wl_surface_set_input_region(m_surface, region);
        wl_region_destroy(region);
        wl_surface_commit(m_surface);
    }

    void WaylandSurface::commit()
    {
        if (m_surface)
        {
            wl_surface_commit(m_surface);
        }
    }

    void WaylandSurface::init()
    {
        init_display();
    }

    void WaylandSurface::resize_buffer(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;

        // 1. Cairo/SHM data allocation (Hybrid mode: we still draw with Cairo to CPU)
        if (m_data)
        {
            munmap(m_data, m_mapped_size);
            m_data = nullptr;
        }

        m_width = width;
        m_height = height;
        m_mapped_size = (size_t)m_width * m_height * 4;

        int stride = m_width * 4;
        int size = (int)m_mapped_size;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        if (fd < 0)
        {
            LOG_ERROR << "[SURFACE] memfd_create failed: " << strerror(errno);
            return;
        }

        if (ftruncate(fd, size) < 0)
        {
            LOG_ERROR << "[SURFACE] ftruncate failed: " << strerror(errno);
            close(fd);
            return;
        }

        m_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if (m_data == MAP_FAILED)
        {
            LOG_ERROR << "[SURFACE] mmap failed: " << strerror(errno);
            m_data = nullptr;
            return;
        }

        // 2. EGL Window and Surface management
        if (!m_egl_window)
        {
            m_egl_window = wl_egl_window_create(m_surface, width, height);
            m_egl_surface = eglCreateWindowSurface(m_egl_display, m_egl_config,
                                                   (EGLNativeWindowType)m_egl_window, nullptr);
        }
        else
        {
            wl_egl_window_resize(m_egl_window, width, height, 0, 0);
        }

        // Make context current for this surface
        eglMakeCurrent(m_egl_display, m_egl_surface, m_egl_surface, m_egl_context);

        if (m_blur_enabled)
        {
            update_blur_region();
        }
    }

    void WaylandSurface::swap_buffers()
    {
        if (m_egl_display != EGL_NO_DISPLAY && m_egl_surface != EGL_NO_SURFACE)
        {
            eglSwapBuffers(m_egl_display, m_egl_surface);
        }
    }

    void WaylandSurface::free()
    {
        if (g_pointer_focus == this)
            g_pointer_focus = nullptr;

        m_configured = false;
        m_listener = nullptr;

        // 1. Role objects (Release handles while connection is alive)
        if (m_xdg_toplevel) { xdg_toplevel_destroy(m_xdg_toplevel); m_xdg_toplevel = nullptr; }
        if (m_xdg_popup) { xdg_popup_destroy(m_xdg_popup); m_xdg_popup = nullptr; }
        if (m_xdg_positioner) { xdg_positioner_destroy(m_xdg_positioner); m_xdg_positioner = nullptr; }
        if (m_layer_surface) { zwlr_layer_surface_v1_destroy(m_layer_surface); m_layer_surface = nullptr; }
        if (m_xdg_surface) { xdg_surface_destroy(m_xdg_surface); m_xdg_surface = nullptr; }

        // 2. Cursor cleanup (needs SHM and Compositor)
        if (m_cursor_theme) { wl_cursor_theme_destroy(m_cursor_theme); m_cursor_theme = nullptr; }
        if (m_cursor_surface) { wl_surface_destroy(m_cursor_surface); m_cursor_surface = nullptr; }

        if (m_egl_display != EGL_NO_DISPLAY && m_egl_surface != EGL_NO_SURFACE) {
            if (eglGetCurrentSurface(EGL_READ) == m_egl_surface || 
                eglGetCurrentSurface(EGL_DRAW) == m_egl_surface) {
                eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            }
            eglDestroySurface(m_egl_display, m_egl_surface);
            m_egl_surface = EGL_NO_SURFACE;
        }

        if (m_egl_window)
        {
            wl_egl_window_destroy(m_egl_window);
            m_egl_window = nullptr;
        }

        // 5. wl_surface
        if (m_surface) { 
            wl_surface_destroy(m_surface); 
            m_surface = nullptr; 
        }

        // 6. Data/SHM map release
        if (m_data) {
            munmap(m_data, m_mapped_size);
            m_data = nullptr;
        }

        // 7. Shared connection (only if owned)
        if (m_owns_connection) {
            if (m_pointer) { wl_pointer_destroy(m_pointer); m_pointer = nullptr; }
            if (m_keyboard) { wl_keyboard_destroy(m_keyboard); m_keyboard = nullptr; }
            if (m_seat) { wl_seat_destroy(m_seat); m_seat = nullptr; }
            if (m_xdg_wm_base) { xdg_wm_base_destroy(m_xdg_wm_base); m_xdg_wm_base = nullptr; }
            if (m_layer_shell) { zwlr_layer_shell_v1_destroy(m_layer_shell); m_layer_shell = nullptr; }
            
            if (m_shm) { wl_shm_destroy(m_shm); m_shm = nullptr; }
            if (m_compositor) { wl_compositor_destroy(m_compositor); m_compositor = nullptr; }
            if (m_registry) { wl_registry_destroy(m_registry); m_registry = nullptr; }

            if (m_egl_display != EGL_NO_DISPLAY) {
                if (m_egl_context != EGL_NO_CONTEXT) {
                    eglDestroyContext(m_egl_display, m_egl_context);
                    m_egl_context = EGL_NO_CONTEXT;
                }
                eglTerminate(m_egl_display);
                m_egl_display = EGL_NO_DISPLAY;
            }

            if (m_display) {
                wl_display_disconnect(m_display);
                m_display = nullptr;
            }
        }
    }

    void WaylandSurface::request_move(uint32_t serial)
    {
        if (m_xdg_toplevel && m_seat)
        {
            xdg_toplevel_move(m_xdg_toplevel, m_seat, serial);
        }
    }

    void WaylandSurface::request_resize(uint32_t serial, uint32_t edge)
    {
        if (m_xdg_toplevel && m_seat)
        {
            xdg_toplevel_resize(m_xdg_toplevel, m_seat, serial, edge);
        }
    }

    void WaylandSurface::set_min_size(int w, int h)
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_min_size(m_xdg_toplevel, w, h);
        }
    }

    void WaylandSurface::set_max_size(int w, int h)
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_max_size(m_xdg_toplevel, w, h);
        }
    }

    void WaylandSurface::request_maximize()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_maximized(m_xdg_toplevel);
            wl_surface_commit(m_surface);
        }
    }

    void WaylandSurface::request_minimize()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_minimized(m_xdg_toplevel);
            wl_surface_commit(m_surface);
        }
    }

    void WaylandSurface::request_restore()
    {
        if (m_xdg_toplevel)
        {
            LOG_INFO << "[SURFACE] Requesting restore (unset maximized)";
            xdg_toplevel_unset_maximized(m_xdg_toplevel);

            // NOTE: xdg_shell doesn't have an "unset_minimized".
            // Some compositors restore when focus is requested or activation occurs.
            // We commit to ensure the request is sent.
            wl_surface_commit(m_surface);
        }
    }

    void WaylandSurface::request_fullscreen()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_set_fullscreen(m_xdg_toplevel, nullptr);
            wl_surface_commit(m_surface);
        }
    }

    void WaylandSurface::request_unfullscreen()
    {
        if (m_xdg_toplevel)
        {
            xdg_toplevel_unset_fullscreen(m_xdg_toplevel);
            wl_surface_commit(m_surface);
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
        case CursorType::ResizeNS:
            names = {"n-resize", "s-resize", "v_double_arrow", "ns-resize", "size_ver"};
            break;
        case CursorType::ResizeEW:
            names = {"e-resize", "w-resize", "h_double_arrow", "ew-resize", "size_hor"};
            break;
        case CursorType::ResizeNESW:
            names = {"ne-resize", "sw-resize", "size_bdiag", "nesw-resize"};
            break;
        case CursorType::ResizeNWSE:
            names = {"nw-resize", "se-resize", "size_fdiag", "nwse-resize"};
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

    bool WaylandSurface::is_fullscreen() const
    {
        return m_is_fullscreen;
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

    void WaylandSurface::move_layer_to_monitor(struct wl_output *output)
    {
        if (m_role != Role::LayerShell || !m_layer_surface)
        {
            LOG_INFO << "move_layer_to_monitor: Ignored (Role: " << (int)m_role
                     << ", Surface: " << m_layer_surface << ")";
            return;
        }

        LOG_INFO << "move_layer_to_monitor: Moving to output " << output
                 << " (Layer: " << m_layer_num << ", NS: " << m_layer_namespace << ")";

        // 1. Reset configuration state
        m_configured = false;

        // 2. Clean up OLD surface and layer surface
        if (m_egl_surface != EGL_NO_SURFACE)
        {
            eglDestroySurface(m_egl_display, m_egl_surface);
            m_egl_surface = EGL_NO_SURFACE;
        }

        if (m_egl_window)
        {
            wl_egl_window_destroy(m_egl_window);
            m_egl_window = nullptr;
        }

        zwlr_layer_surface_v1_destroy(m_layer_surface);
        wl_surface_destroy(m_surface);

        // 3. Recreate EVERYTHING for the new output
        m_surface = wl_compositor_create_surface(m_compositor);
        m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            m_layer_shell, m_surface, output, m_layer_num, m_layer_namespace.c_str());

        if (!m_layer_surface)
        {
            LOG_INFO << "move_layer_to_monitor: FAILED to create new layer surface!";
            return;
        }

        // 4. Re-apply listeners and state
        static const zwlr_layer_surface_v1_listener listener = {
            .configure =
                [](void *data, zwlr_layer_surface_v1 *ls, uint32_t ser, uint32_t w, uint32_t h)
            {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                zwlr_layer_surface_v1_ack_configure(ls, ser);
                if (w > 0 && h > 0)
                {
                    self->resize_buffer(w, h); // This will attach m_buffer to NEW m_surface
                    if (self->m_listener)
                        self->m_listener->on_resize(w, h);
                }
                wl_surface_commit(self->m_surface);
            },
            .closed = [](void *, zwlr_layer_surface_v1 *) {}};

        zwlr_layer_surface_v1_add_listener(m_layer_surface, &listener, this);
        zwlr_layer_surface_v1_set_anchor(m_layer_surface, m_anchor);
        zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, m_exclusive_zone);
        zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, m_interactivity);
        zwlr_layer_surface_v1_set_size(m_layer_surface, (uint32_t)m_width, (uint32_t)m_height);

        // 5. Initial commit (WITHOUT buffer) to trigger the first configure
        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);
    }

    void WaylandSurface::add_wl_output(struct wl_output *output)
    {
        LOG_INFO << "Added Wayland output: " << output << " (Total: " << m_outputs.size() + 1
                 << ")";
        m_outputs.push_back(output);
        m_monitor_details.push_back({output, {}});

        // Registrar listener para capturar el tamaño del monitor
        static const wl_output_listener s_output_listener = {
            // geometry
            [](void *, wl_output *, int32_t, int32_t, int32_t, int32_t, int32_t,
               const char *, const char *, int32_t) {},
            // mode: capturamos width y height de todos los modos
            [](void *data, wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
            {
                auto *self = static_cast<WaylandSurface *>(data);
                
                // Buscar el MonitorDetail correspondiente
                auto it = std::find_if(self->m_monitor_details.begin(), self->m_monitor_details.end(),
                                       [output](const MonitorDetail &d) { return d.output == output; });
                
                if (it != self->m_monitor_details.end()) {
                    MonitorModeInfo mode;
                    mode.width = width;
                    mode.height = height;
                    mode.refresh = refresh; // in mHz
                    mode.current = (flags & WL_OUTPUT_MODE_CURRENT);
                    mode.preferred = (flags & WL_OUTPUT_MODE_PREFERRED);
                    it->modes.push_back(mode);
                    
                    LOG_INFO << "[SURFACE] Mode received for output " << output << ": " 
                             << width << "x" << height << " @ " << (refresh/1000.0f) << "Hz"
                             << (mode.current ? " (current)" : "") 
                             << (mode.preferred ? " (preferred)" : "");
                }

                if (flags & WL_OUTPUT_MODE_CURRENT)
                {
                    self->m_monitor_width  = width;
                    self->m_monitor_height = height;
                    LOG_INFO << "[SURFACE] Monitor size from wl_output mode: "
                             << width << "x" << height;
                }
            },
            // done
            [](void *, wl_output *) {},
            // scale
            [](void *, wl_output *, int32_t) {},
        };
        wl_output_add_listener(output, &s_output_listener, this);
    }

    struct ActivationData
    {
        std::function<void(const std::string &)> callback;
        xdg_activation_token_v1 *token_obj;
    };

    static void token_handle_done(void *data, struct xdg_activation_token_v1 *token_obj,
                                  const char *token)
    {
        auto *act_data = static_cast<ActivationData *>(data);
        LOG_INFO << "[SURFACE] Activation token received: " << (token ? token : "NULL")
                 << " (Obj: " << (void *)token_obj << ")";
        if (act_data->callback)
        {
            act_data->callback(token ? token : "");
        }
        xdg_activation_token_v1_destroy(token_obj);
        delete act_data;
    }

    static const struct xdg_activation_token_v1_listener token_listener = {
        .done = token_handle_done,
    };

    void WaylandSurface::request_activation_token(std::function<void(const std::string &)> callback,
                                                  uint32_t serial)
    {
        if (!m_activation)
        {
            if (callback)
                callback("");
            return;
        }

        uint32_t use_serial = (serial != 0) ? serial : m_last_serial;
        LOG_INFO << "[SURFACE] Requesting activation token (Serial: " << use_serial << ")";

        auto *act_data = new ActivationData();
        act_data->callback = callback;
        act_data->token_obj = xdg_activation_v1_get_activation_token(m_activation);

        xdg_activation_token_v1_add_listener(act_data->token_obj, &token_listener, act_data);
        xdg_activation_token_v1_set_serial(act_data->token_obj, use_serial, m_seat);
        xdg_activation_token_v1_set_surface(act_data->token_obj, m_surface);
        xdg_activation_token_v1_commit(act_data->token_obj);

        wl_display_flush(m_display);
    }

    void WaylandSurface::activate(const std::string &token)
    {
        if (!m_activation || token.empty())
            return;
        xdg_activation_v1_activate(m_activation, token.c_str(), m_surface);
        commit(); // Ensure activation is sent
        wl_display_flush(m_display);
    }


    void WaylandSurface::activate_foreign_app(const std::string &app_id)
    {
        if (!m_foreign_toplevel_manager)
            return;

        LOG_INFO << "[SURFACE] Attempting foreign activation for: " << app_id;
        for (auto &pair : m_foreign_toplevels)
        {
            auto &ft = pair.second;
            if (ft.app_id == app_id)
            {
                LOG_INFO << "[SURFACE] Found foreign handle for " << app_id << ", activating.";
                if (m_seat)
                {
                    zwlr_foreign_toplevel_handle_v1_activate(ft.handle, m_seat);
                }
            }
        }
    }

    void WaylandSurface::minimize_foreign_app(const std::string &app_id)
    {
        if (!m_foreign_toplevel_manager)
            return;

        LOG_INFO << "[SURFACE] Attempting foreign minimize for: " << app_id;
        for (auto &pair : m_foreign_toplevels)
        {
            auto &ft = pair.second;
            if (ft.app_id == app_id)
            {
                LOG_INFO << "[SURFACE] Found foreign handle for " << app_id << ", minimizing.";
                zwlr_foreign_toplevel_handle_v1_set_minimized(ft.handle);
            }
        }
    }

    void WaylandSurface::toggle_fullscreen_foreign_app(const std::string &app_id)
    {
        if (!m_foreign_toplevel_manager)
            return;

        LOG_INFO << "[SURFACE] Attempting foreign toggle fullscreen for: " << app_id;
        for (auto &pair : m_foreign_toplevels)
        {
            auto &ft = pair.second;
            if (ft.app_id == app_id)
            {
                LOG_INFO << "[SURFACE] Found foreign handle for " << app_id << ", setting fullscreen.";
                zwlr_foreign_toplevel_handle_v1_set_fullscreen(ft.handle, nullptr);
            }
        }
    }

    void WaylandSurface::close_foreign_app(const std::string &app_id)
    {
        if (!m_foreign_toplevel_manager)
            return;

        LOG_INFO << "[SURFACE] Attempting foreign close for: " << app_id;
        for (auto &pair : m_foreign_toplevels)
        {
            auto &ft = pair.second;
            if (ft.app_id == app_id)
            {
                LOG_INFO << "[SURFACE] Found foreign handle for " << app_id << ", closing.";
                zwlr_foreign_toplevel_handle_v1_close(ft.handle);
            }
        }
    }

    void WaylandSurface::activate_foreign_instance(uintptr_t instance_id)
    {
        if (instance_id == 0)
            return;

        // Try zwlr protocol
        auto *zwlr_handle = reinterpret_cast<struct zwlr_foreign_toplevel_handle_v1 *>(instance_id);
        if (m_foreign_toplevel_manager && m_foreign_toplevels.count(zwlr_handle) && m_seat)
        {
            LOG_INFO << "[SURFACE] Activating foreign instance (zwlr): " << instance_id;
            zwlr_foreign_toplevel_handle_v1_activate(zwlr_handle, m_seat);
            return;
        }

        // Try modern ext protocol (Note: ext protocol might not have a direct 'activate' yet in shared library version, 
        // but we should at least check for its existence or implement if available in our protocols)
        auto *ext_handle = reinterpret_cast<struct ext_foreign_toplevel_handle_v1 *>(instance_id);
        if (m_ext_foreign_toplevel_list && m_ext_foreign_toplevels.count(ext_handle))
        {
             LOG_INFO << "[SURFACE] Target instance is ext-foreign-toplevel: " << instance_id;
             // ext-foreign-level-list-v1 is often for listing only, activation might need xdg_activation_v1
        }
    }

    void WaylandSurface::minimize_foreign_instance(uintptr_t instance_id)
    {
        if (instance_id == 0)
            return;

        auto *zwlr_handle = reinterpret_cast<struct zwlr_foreign_toplevel_handle_v1 *>(instance_id);
        if (m_foreign_toplevel_manager && m_foreign_toplevels.count(zwlr_handle))
        {
            LOG_INFO << "[SURFACE] Minimizing foreign instance (zwlr): " << instance_id;
            zwlr_foreign_toplevel_handle_v1_set_minimized(zwlr_handle);
            return;
        }
    }

    void WaylandSurface::toggle_fullscreen_foreign_instance(uintptr_t instance_id)
    {
        if (instance_id == 0)
            return;

        auto *zwlr_handle = reinterpret_cast<struct zwlr_foreign_toplevel_handle_v1 *>(instance_id);
        if (m_foreign_toplevel_manager && m_foreign_toplevels.count(zwlr_handle))
        {
            LOG_INFO << "[SURFACE] Toggling fullscreen for foreign instance (zwlr): " << instance_id;
            zwlr_foreign_toplevel_handle_v1_set_fullscreen(zwlr_handle, nullptr);
            return;
        }
    }

    void WaylandSurface::close_foreign_instance(uintptr_t instance_id)
    {
        if (instance_id == 0)
            return;

        auto *zwlr_handle = reinterpret_cast<struct zwlr_foreign_toplevel_handle_v1 *>(instance_id);
        if (m_foreign_toplevel_manager && m_foreign_toplevels.count(zwlr_handle))
        {
            LOG_INFO << "[SURFACE] Closing foreign instance (zwlr): " << instance_id;
            zwlr_foreign_toplevel_handle_v1_close(zwlr_handle);
            return;
        }

        auto *ext_handle = reinterpret_cast<struct ext_foreign_toplevel_handle_v1 *>(instance_id);
        if (m_ext_foreign_toplevel_list && m_ext_foreign_toplevels.count(ext_handle))
        {
            LOG_INFO << "[SURFACE] Closing foreign instance (ext): " << instance_id;
            // ext_foreign_toplevel_handle_v1 doesn't have a close request, it's read-only for now
            // according to the current protocol implemented in some compositors. 
            // In others, it might have it. we'll leave it as a log for now or if we bound it.
        }
    }

    // Foreign toplevel management callbacks implementation
    void foreign_toplevel_handle_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                       const char *title)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        ws->m_foreign_toplevels[handle].title = title ? title : "";
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void foreign_toplevel_handle_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                        const char *app_id)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] Foreign toplevel app_id: " << (app_id ? app_id : "NULL");
        ws->m_foreign_toplevels[handle].app_id = app_id ? app_id : "";
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void foreign_toplevel_handle_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                       struct wl_array *state)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        uint32_t *entry;
        bool minimized = false;
        bool active = false;
        // Using common Wayland loop pattern
        uint32_t *states_data = static_cast<uint32_t *>(state->data);
        size_t states_count = state->size / sizeof(uint32_t);
        for (size_t i = 0; i < states_count; ++i)
        {
            uint32_t s = states_data[i];
            if (s == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)
                minimized = true;
            if (s == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)
                active = true;
        }
        ws->m_foreign_toplevels[handle].minimized = minimized;
        ws->m_foreign_toplevels[handle].active = active;
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void foreign_toplevel_handle_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] Foreign toplevel closed: " << ws->m_foreign_toplevels[handle].app_id;
        ws->m_foreign_toplevels.erase(handle);
        zwlr_foreign_toplevel_handle_v1_destroy(handle);
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void foreign_toplevel_handle_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
                                        struct zwlr_foreign_toplevel_handle_v1 *parent)
    {
        // Dummy handler
    }

    void foreign_toplevel_handle_output_enter(void *data,
                                              struct zwlr_foreign_toplevel_handle_v1 *handle,
                                              struct wl_output *output)
    {
        // Dummy handler
    }

    void foreign_toplevel_handle_output_leave(void *data,
                                              struct zwlr_foreign_toplevel_handle_v1 *handle,
                                              struct wl_output *output)
    {
        // Dummy handler
    }

    void foreign_toplevel_handle_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void foreign_toplevel_manager_finished(void *data,
                                           struct zwlr_foreign_toplevel_manager_v1 *manager)
    {
        // Dummy handler
    }

    void foreign_toplevel_manager_toplevel(void *data,
                                           struct zwlr_foreign_toplevel_manager_v1 *manager,
                                           struct zwlr_foreign_toplevel_handle_v1 *handle)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] New foreign toplevel handle discovered";
        zwlr_foreign_toplevel_handle_v1_add_listener(handle, &foreign_toplevel_handle_listener, ws);
    }

    void ext_foreign_toplevel_list_handle_toplevel(void *data,
                                                   struct ext_foreign_toplevel_list_v1 *ext_foreign_toplevel_list_v1,
                                                   struct ext_foreign_toplevel_handle_v1 *handle)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        ws->m_ext_foreign_toplevels[handle].ext_handle = handle;
        LOG_INFO << "[SURFACE] New modern ext-foreign toplevel handle discovered";
        ext_foreign_toplevel_handle_v1_add_listener(handle, &ext_foreign_toplevel_handle_listener, ws);
    }

    void ext_foreign_toplevel_handle_app_id(void *data,
                                            struct ext_foreign_toplevel_handle_v1 *handle,
                                            const char *app_id)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] Ext-Foreign toplevel app_id: " << (app_id ? app_id : "NULL");
        ws->m_ext_foreign_toplevels[handle].app_id = app_id ? app_id : "";
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void ext_foreign_toplevel_handle_title(void *data,
                                           struct ext_foreign_toplevel_handle_v1 *handle,
                                           const char *title)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        ws->m_ext_foreign_toplevels[handle].title = title ? title : "";
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void ext_foreign_toplevel_handle_closed(void *data,
                                            struct ext_foreign_toplevel_handle_v1 *handle)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] Ext-Foreign toplevel closed: " << ws->m_ext_foreign_toplevels[handle].app_id;
        ws->m_ext_foreign_toplevels.erase(handle);
        ext_foreign_toplevel_handle_v1_destroy(handle);
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void ext_foreign_toplevel_handle_done(void *data,
                                          struct ext_foreign_toplevel_handle_v1 *handle)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        if (ws->m_listener)
            ws->m_listener->on_foreign_toplevel_event();
    }

    void WaylandSurface::update_blur_region()
    {
        if (!m_surface || !m_blur_enabled)
            return;

        if (m_width <= 0 || m_height <= 0)
            return;

        // Try ext-background-effect-v1 (standard/modern)
        if (m_background_effect_manager)
        {
            if (!m_background_effect_surface)
            {
                m_background_effect_surface =
                    ext_background_effect_manager_v1_get_background_effect(
                        m_background_effect_manager, m_surface);
            }

            struct wl_region *region = wl_compositor_create_region(m_compositor);
            wl_region_add(region, 0, 0, m_width, m_height);
            ext_background_effect_surface_v1_set_blur_region(m_background_effect_surface, region);
            wl_region_destroy(region);
            LOG_INFO << "[SURFACE] Blur region updated via ext-background-effect (" << m_width
                     << "x" << m_height << ")";
        }
        // Try org_kde_kwin_blur_manager (KDE/Wayfire fallback)
        else if (m_blur_manager)
        {
            if (!m_blur_object)
            {
                m_blur_object = org_kde_kwin_blur_manager_create(m_blur_manager, m_surface);
            }

            struct wl_region *region = wl_compositor_create_region(m_compositor);
            wl_region_add(region, 0, 0, m_width, m_height);
            org_kde_kwin_blur_set_region(m_blur_object, region);
            org_kde_kwin_blur_commit(m_blur_object);
            wl_region_destroy(region);
            LOG_INFO << "[SURFACE] Blur region updated via org_kde_kwin_blur (" << m_width << "x"
                     << m_height << ")";
        }
    }

    void WaylandSurface::set_blur(bool enabled)
    {
        if (m_blur_enabled == enabled)
            return;

        m_blur_enabled = enabled;

        if (!m_surface)
            return;

        if (enabled)
        {
            update_blur_region();
        }
        else
        {
            if (m_background_effect_surface)
            {
                ext_background_effect_surface_v1_destroy(m_background_effect_surface);
                m_background_effect_surface = nullptr;
            }

            if (m_blur_object)
            {
                org_kde_kwin_blur_release(m_blur_object);
                m_blur_object = nullptr;
            }
            LOG_INFO << "[SURFACE] Blur disabled";
        }

        wl_surface_commit(m_surface);
    }
} // namespace horizon
