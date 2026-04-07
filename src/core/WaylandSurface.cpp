#include "horizon/WaylandSurface.hpp"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <horizon/Logger.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <horizon/xdg-activation-v1-client-protocol.h>
#include <horizon/xdg-shell-client-protocol.h>
#include <protocols/blur-client-protocol.h>
#include <protocols/ext-background-effect-v1-client-protocol.h>
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

    // --- Static Handlers Forward Declarations ---
    static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps);
    static void seat_handle_name(void *data, wl_seat *seat, const char *name);
    
    static void pointer_handle_enter(void *data, wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_handle_leave(void *data, wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);
    static void pointer_handle_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_handle_button(void *data, wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    static void pointer_handle_axis(void *data, wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);

    static void keyboard_handle_keymap(void *data, wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size);
    static void keyboard_handle_enter(void *data, wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
    static void keyboard_handle_leave(void *data, wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
    static void keyboard_handle_key(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    static void keyboard_handle_repeat_info(void *data, wl_keyboard *keyboard, int32_t rate, int32_t delay);

    // Friend functions must NOT be static in the .cpp file as they need external linkage
    void foreign_toplevel_handle_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, const char *title);
    void foreign_toplevel_handle_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, const char *app_id);
    void foreign_toplevel_handle_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_array *state);
    void foreign_toplevel_handle_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle);
    void foreign_toplevel_handle_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle);
    void foreign_toplevel_handle_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_output *output);
    void foreign_toplevel_handle_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_output *output);
    void foreign_toplevel_handle_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct zwlr_foreign_toplevel_handle_v1 *parent);
    void foreign_toplevel_manager_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *manager, struct zwlr_foreign_toplevel_handle_v1 *handle);
    void registry_global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);

    // --- Listeners ---
    static const wl_pointer_listener g_pointer_listener = {
        pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button, pointer_handle_axis};

    static const wl_keyboard_listener g_keyboard_listener = {
        keyboard_handle_keymap, keyboard_handle_enter, keyboard_handle_leave, keyboard_handle_key, keyboard_handle_modifiers, keyboard_handle_repeat_info};

    static const wl_seat_listener g_seat_listener = {seat_handle_capabilities, seat_handle_name};

    static const struct zwlr_foreign_toplevel_handle_v1_listener foreign_toplevel_handle_listener = {
        foreign_toplevel_handle_title,        foreign_toplevel_handle_app_id,
        foreign_toplevel_handle_output_enter, foreign_toplevel_handle_output_leave,
        foreign_toplevel_handle_state,        foreign_toplevel_handle_done,
        foreign_toplevel_handle_closed,       foreign_toplevel_handle_parent
    };

    static const struct zwlr_foreign_toplevel_manager_v1_listener foreign_toplevel_manager_listener = {
        foreign_toplevel_manager_toplevel,
        nullptr // finished
    };

    // --- Output Handlers ---
    void output_handle_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
                                      int32_t physical_width, int32_t physical_height, int32_t subpixel,
                                      const char *make, const char *model, int32_t transform);
    void output_handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
                                  int32_t width, int32_t height, int32_t refresh);
    void output_handle_done(void *data, struct wl_output *wl_output);
    void output_handle_scale(void *data, struct wl_output *wl_output, int32_t factor);
    void output_handle_name(void *data, struct wl_output *wl_output, const char *name);
    void output_handle_description(void *data, struct wl_output *wl_output, const char *description);

    static const struct wl_output_listener output_listener = {
        output_handle_geometry,
        output_handle_mode,
        output_handle_done,
        output_handle_scale,
        output_handle_name,
        output_handle_description,
    };

    // --- Implementation ---

    WaylandSurface::WaylandSurface(int w, int h) : m_width(w), m_height(h)
    {
        m_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    WaylandSurface::~WaylandSurface()
    {
        if (g_pointer_focus == this) g_pointer_focus = nullptr;
        free();
        if (m_xkb_state) xkb_state_unref(m_xkb_state);
        if (m_xkb_keymap) xkb_keymap_unref(m_xkb_keymap);
        if (m_xkb_context) xkb_context_unref(m_xkb_context);
    }

    void WaylandSurface::set_wl_compositor(struct wl_compositor *compositor) { m_compositor = compositor; }
    void WaylandSurface::set_wl_shm(struct wl_shm *shm) { m_shm = shm; }
    void WaylandSurface::set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base) { m_xdg_wm_base = xdg_wm_base; }
    void WaylandSurface::set_zwlr_layer_shell(struct zwlr_layer_shell_v1 *layer_shell) { m_layer_shell = layer_shell; }
    void WaylandSurface::set_wl_seat(struct wl_seat *seat) { m_seat = seat; }
    void WaylandSurface::set_wl_pointer(struct wl_pointer *pointer) { m_pointer = pointer; }
    void WaylandSurface::set_wl_keyboard(struct wl_keyboard *keyboard) { m_keyboard = keyboard; }
    void WaylandSurface::set_xdg_activation(struct xdg_activation_v1 *activation) { m_activation = activation; }
    void WaylandSurface::set_zwlr_foreign_toplevel_manager(struct zwlr_foreign_toplevel_manager_v1 *manager) { m_foreign_toplevel_manager = manager; }
    void WaylandSurface::set_ext_background_effect_manager(struct ext_background_effect_manager_v1 *manager) { m_background_effect_manager = manager; }
    void WaylandSurface::set_event_listener(WaylandEventListener *listener) { m_listener = listener; }
    void WaylandSurface::set_pointer_x(double x) { m_pointer_x = x; }
    void WaylandSurface::set_pointer_y(double y) { m_pointer_y = y; }

    struct wl_output *WaylandSurface::get_monitor(size_t index) const {
        if (index < m_outputs.size()) return m_outputs[index];
        return nullptr;
    }

    void WaylandSurface::move_layer_to_monitor(struct wl_output *output) {
        if (m_role == Role::LayerShell && m_layer_surface && output) {
            zwlr_layer_surface_v1_set_margin(m_layer_surface, 0, 0, 0, 0); // Reset margins
            // Note: The actual move is often handled by re-creating the surface on the new output
            // but in some protocols it might be different. Let's provide a basic implementation.
            // In wlr-layer-shell, the output is specified at creation time.
            // If the user wants to move it, we might need to signal the change.
            if (m_listener) m_listener->on_resize(m_width, m_height);
        }
    }

    void WaylandSurface::add_wl_output(struct wl_output *output) {
        if (!output) return;
        m_outputs.push_back(output);
        // We could add a listener here to get monitor details if needed
    }

    void WaylandSurface::init_display()
    {
        if (m_display) return;
        m_display = wl_display_connect(nullptr);
        if (!m_display) throw std::runtime_error("Failed to connect to Wayland display");

        m_registry = wl_display_get_registry(m_display);
        static const wl_registry_listener listener = {registry_global, nullptr};
        wl_registry_add_listener(m_registry, &listener, this);

        wl_display_roundtrip(m_display);
        wl_display_roundtrip(m_display); 
        init_egl();
    }

    void WaylandSurface::share_connection_from(WaylandSurface *other)
    {
        if (!other) return;
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
        m_monitor_details = other->m_monitor_details;
        m_egl_display = other->m_egl_display;
        m_egl_config = other->m_egl_config;
        m_egl_context = other->m_egl_context;
    }

    void WaylandSurface::init_egl()
    {
        m_egl_display = eglGetDisplay((EGLNativeDisplayType)m_display);
        if (m_egl_display == EGL_NO_DISPLAY) throw std::runtime_error("Failed to get EGL display");
        if (!eglInitialize(m_egl_display, nullptr, nullptr)) throw std::runtime_error("Failed to initialize EGL");

        EGLint attr[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
        EGLint num_configs;
        if (!eglChooseConfig(m_egl_display, attr, &m_egl_config, 1, &num_configs) || num_configs < 1) throw std::runtime_error("Failed to choose EGL config");

        EGLint ctx_attr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        m_egl_context = eglCreateContext(m_egl_display, m_egl_config, EGL_NO_CONTEXT, ctx_attr);
        if (m_egl_context == EGL_NO_CONTEXT) throw std::runtime_error("Failed to create EGL context");
    }

    void WaylandSurface::setup_xdg_toplevel(const std::string &title, const std::string &app_id)
    {
        m_role = Role::XdgToplevel;
        m_surface = wl_compositor_create_surface(m_compositor);
        wl_surface_set_user_data(m_surface, this);

        m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
        static const xdg_surface_listener xdg_surf_ptr = {
            .configure = [](void *data, xdg_surface *xdg_s, uint32_t serial) {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                xdg_surface_ack_configure(xdg_s, serial);
            }};
        xdg_surface_add_listener(m_xdg_surface, &xdg_surf_ptr, this);

        m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
        xdg_toplevel_set_title(m_xdg_toplevel, title.c_str());
        xdg_toplevel_set_app_id(m_xdg_toplevel, app_id.c_str());

        static const xdg_toplevel_listener toplevel_list = {
            .configure = [](void *data, xdg_toplevel *, int32_t width, int32_t height, struct wl_array *states) {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                uint32_t *state;
                bool maximized = false; bool activated = false; bool fullscreen = false;
                for (state = static_cast<uint32_t *>(states->data); reinterpret_cast<const char *>(state) < (static_cast<const char *>(states->data) + states->size); state++) {
                    if (*state == XDG_TOPLEVEL_STATE_MAXIMIZED) maximized = true;
                    if (*state == XDG_TOPLEVEL_STATE_ACTIVATED) activated = true;
                    if (*state == XDG_TOPLEVEL_STATE_FULLSCREEN) fullscreen = true;
                }
                self->m_is_maximized = maximized; self->m_is_fullscreen = fullscreen;
                if (self->m_is_activated != activated) {
                    self->m_is_activated = activated;
                    if (self->m_listener) self->m_listener->on_activated(activated);
                }
                if (width > 0 && height > 0) {
                    self->resize_buffer(width, height);
                    if (self->m_listener) self->m_listener->on_resize(width, height);
                }
            },
            .close = [](void *data, xdg_toplevel *) {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                if (self->m_listener) self->m_listener->on_close();
            }};
        xdg_toplevel_add_listener(m_xdg_toplevel, &toplevel_list, this);

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        if (m_shm) m_cursor_theme = wl_cursor_theme_load(nullptr, 24, m_shm);
        if (m_compositor) m_cursor_surface = wl_compositor_create_surface(m_compositor);

        resize_buffer(m_width, m_height);
    }

    void WaylandSurface::setup_layer_surface(uint32_t layer, const std::string &namespace_id, struct wl_output *output)
    {
        m_role = Role::LayerShell;
        m_layer_num = layer;
        m_layer_namespace = namespace_id;
        m_surface = wl_compositor_create_surface(m_compositor);
        wl_surface_set_user_data(m_surface, this);

        if (!m_layer_shell) throw std::runtime_error("Compositor does not support wlr-layer-shell");

        m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(m_layer_shell, m_surface, output, layer, namespace_id.c_str());

        static const zwlr_layer_surface_v1_listener layer_surface_listener = {
            .configure = [](void *data, zwlr_layer_surface_v1 *ls, uint32_t serial, uint32_t width, uint32_t height) {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                self->m_configured = true;
                zwlr_layer_surface_v1_ack_configure(ls, serial);
                if (width > 0 && height > 0) {
                    self->resize_buffer(width, height);
                    if (self->m_listener) self->m_listener->on_resize(width, height);
                }
            },
            .closed = [](void *data, zwlr_layer_surface_v1 *) {
                WaylandSurface *self = static_cast<WaylandSurface *>(data);
                if (self->m_listener) self->m_listener->on_close();
            }};
        zwlr_layer_surface_v1_add_listener(m_layer_surface, &layer_surface_listener, this);

        zwlr_layer_surface_v1_set_anchor(m_layer_surface, m_anchor);
        zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, m_exclusive_zone);
        zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, m_interactivity);
        zwlr_layer_surface_v1_set_size(m_layer_surface, (uint32_t)m_width, (uint32_t)m_height);

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        if (m_shm) m_cursor_theme = wl_cursor_theme_load(nullptr, 24, m_shm);
        if (m_compositor) m_cursor_surface = wl_compositor_create_surface(m_compositor);
        resize_buffer(m_width, m_height);
    }

    void WaylandSurface::setup_xdg_popup(WaylandSurface *parent, int x, int y, int w, int h)
    {
        m_role = Role::XdgPopup;
        share_connection_from(parent);
        m_width = w; m_height = h;

        m_surface = wl_compositor_create_surface(m_compositor);
        wl_surface_set_user_data(m_surface, this);
        resize_buffer(w, h);

        m_xdg_positioner = xdg_wm_base_create_positioner(m_xdg_wm_base);
        xdg_positioner_set_size(m_xdg_positioner, w, h);
        xdg_positioner_set_anchor_rect(m_xdg_positioner, x, y, 1, 1);
        xdg_positioner_set_anchor(m_xdg_positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
        xdg_positioner_set_gravity(m_xdg_positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
        xdg_positioner_set_constraint_adjustment(m_xdg_positioner, 
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X | 
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y | 
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y);

        if (parent->m_xdg_surface || parent->m_layer_surface) {
            m_xdg_surface = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
            static const struct xdg_surface_listener popup_xdg_surf_listener = {
                .configure = [](void *data, struct xdg_surface *surf, uint32_t serial) {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    self->m_configured = true;
                    xdg_surface_ack_configure(surf, serial);
                }};
            xdg_surface_add_listener(m_xdg_surface, &popup_xdg_surf_listener, this);

            m_xdg_popup = xdg_surface_get_popup(m_xdg_surface, parent->m_xdg_surface, m_xdg_positioner);
            if (parent->m_layer_surface) zwlr_layer_surface_v1_get_popup(parent->m_layer_surface, m_xdg_popup);
        }

        if (m_xdg_popup) {
            static const struct xdg_popup_listener popup_listener = {
                .configure = [](void *data, struct xdg_popup *, int32_t, int32_t, int32_t width, int32_t height) {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    if (width > 0 && height > 0) {
                        self->resize_buffer(width, height);
                        if (self->m_listener) self->m_listener->on_resize(width, height);
                    }
                },
                .popup_done = [](void *data, struct xdg_popup *) {
                    WaylandSurface *self = static_cast<WaylandSurface *>(data);
                    if (self->m_listener) self->m_listener->on_close();
                }};
            xdg_popup_add_listener(m_xdg_popup, &popup_listener, this);
            if (m_seat) xdg_popup_grab(m_xdg_popup, m_seat, parent->last_serial());
        }

        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);
        resize_buffer(m_width, m_height);
    }

    void WaylandSurface::set_layer_anchor(uint32_t anchor) { m_anchor = anchor; if (m_layer_surface) zwlr_layer_surface_v1_set_anchor(m_layer_surface, anchor); }
    void WaylandSurface::set_layer_exclusive_zone(int32_t zone) { m_exclusive_zone = zone; if (m_layer_surface) zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, zone); }
    void WaylandSurface::set_layer_keyboard_interactivity(uint32_t interactivity) { m_interactivity = interactivity; if (m_layer_surface) zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, interactivity); }
    void WaylandSurface::set_layer_size(uint32_t width, uint32_t height) { if (m_data == nullptr) { if (width > 0) m_width = (int)width; if (height > 0) m_height = (int)height; } if (m_layer_surface) zwlr_layer_surface_v1_set_size(m_layer_surface, width, height); }

    void WaylandSurface::set_input_region(int x, int y, int w, int h) {
        if (!m_surface || !m_compositor) return;
        struct wl_region *region = wl_compositor_create_region(m_compositor);
        wl_region_add(region, x, y, w, h);
        wl_surface_set_input_region(m_surface, region);
        wl_region_destroy(region);
        wl_surface_commit(m_surface);
    }

    void WaylandSurface::clear_input_region() {
        if (!m_surface || !m_compositor) return;
        struct wl_region *region = wl_compositor_create_region(m_compositor);
        wl_surface_set_input_region(m_surface, region);
        wl_region_destroy(region);
        wl_surface_commit(m_surface);
    }

    void WaylandSurface::init() { init_display(); }
    void WaylandSurface::commit() { if (m_surface) wl_surface_commit(m_surface); }

    void WaylandSurface::resize_buffer(int width, int height)
    {
        LOG_INFO << "[SURFACE] resize_buffer: " << width << "x" << height;
        if (width <= 0 || height <= 0) return;
        if (m_width == width && m_height == height && m_data) return;

        if (m_data) { munmap(m_data, m_mapped_size); m_data = nullptr; }
        m_width = width; m_height = height; m_mapped_size = (size_t)m_width * m_height * 4;
        int size = (int)m_mapped_size;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        if (fd < 0) return;
        if (ftruncate(fd, size) < 0) { close(fd); return; }
        m_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (m_data == MAP_FAILED) { 
            LOG_ERROR << "[SURFACE] mmap failed";
            m_data = nullptr; return; 
        }
        LOG_INFO << "[SURFACE] Buffer mapped at " << m_data;

        if (!m_egl_window) {
            LOG_INFO << "[SURFACE] Creating EGL window/surface...";
            m_egl_window = wl_egl_window_create(m_surface, width, height);
            m_egl_surface = eglCreateWindowSurface(m_egl_display, m_egl_config, (EGLNativeWindowType)m_egl_window, nullptr);
        } else {
            wl_egl_window_resize(m_egl_window, width, height, 0, 0);
        }
        LOG_INFO << "[SURFACE] Making EGL context current...";
        eglMakeCurrent(m_egl_display, m_egl_surface, m_egl_surface, m_egl_context);
        if (m_blur_enabled) update_blur_region();
        LOG_INFO << "[SURFACE] resize_buffer done";
    }

    void WaylandSurface::swap_buffers() { if (m_egl_display != EGL_NO_DISPLAY && m_egl_surface != EGL_NO_SURFACE) eglSwapBuffers(m_egl_display, m_egl_surface); }

    void WaylandSurface::free()
    {
        if (g_pointer_focus == this) g_pointer_focus = nullptr;
        m_configured = false; m_listener = nullptr;
        if (m_xdg_toplevel) { xdg_toplevel_destroy(m_xdg_toplevel); m_xdg_toplevel = nullptr; }
        if (m_xdg_popup) { xdg_popup_destroy(m_xdg_popup); m_xdg_popup = nullptr; }
        if (m_xdg_positioner) { xdg_positioner_destroy(m_xdg_positioner); m_xdg_positioner = nullptr; }
        if (m_layer_surface) { zwlr_layer_surface_v1_destroy(m_layer_surface); m_layer_surface = nullptr; }
        if (m_xdg_surface) { xdg_surface_destroy(m_xdg_surface); m_xdg_surface = nullptr; }
        if (m_cursor_theme) { wl_cursor_theme_destroy(m_cursor_theme); m_cursor_theme = nullptr; }
        if (m_cursor_surface) { wl_surface_destroy(m_cursor_surface); m_cursor_surface = nullptr; }

        if (m_egl_display != EGL_NO_DISPLAY && m_egl_surface != EGL_NO_SURFACE) {
            eglMakeCurrent(m_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroySurface(m_egl_display, m_egl_surface);
            m_egl_surface = EGL_NO_SURFACE;
        }
        if (m_egl_window) { wl_egl_window_destroy(m_egl_window); m_egl_window = nullptr; }
        if (m_surface) { wl_surface_destroy(m_surface); m_surface = nullptr; }
        if (m_data) { munmap(m_data, m_mapped_size); m_data = nullptr; }

        if (m_owns_connection) {
            if (m_data_device) { wl_data_device_release(m_data_device); m_data_device = nullptr; }
            if (m_data_device_manager) { wl_data_device_manager_destroy(m_data_device_manager); m_data_device_manager = nullptr; }
            if (m_pointer) { wl_pointer_destroy(m_pointer); m_pointer = nullptr; }

            if (m_keyboard) { wl_keyboard_destroy(m_keyboard); m_keyboard = nullptr; }
            if (m_seat) { wl_seat_destroy(m_seat); m_seat = nullptr; }
            if (m_xdg_wm_base) { xdg_wm_base_destroy(m_xdg_wm_base); m_xdg_wm_base = nullptr; }
            if (m_layer_shell) { zwlr_layer_shell_v1_destroy(m_layer_shell); m_layer_shell = nullptr; }
            if (m_shm) { wl_shm_destroy(m_shm); m_shm = nullptr; }
            if (m_compositor) { wl_compositor_destroy(m_compositor); m_compositor = nullptr; }
            if (m_registry) { wl_registry_destroy(m_registry); m_registry = nullptr; }
            if (m_egl_display != EGL_NO_DISPLAY) {
                if (m_egl_context != EGL_NO_CONTEXT) { eglDestroyContext(m_egl_display, m_egl_context); m_egl_context = EGL_NO_CONTEXT; }
                eglTerminate(m_egl_display);
                m_egl_display = EGL_NO_DISPLAY;
            }
            if (m_display) { wl_display_disconnect(m_display); m_display = nullptr; }
        }
    }

    void WaylandSurface::request_move(uint32_t serial) { if (m_xdg_toplevel && m_seat) xdg_toplevel_move(m_xdg_toplevel, m_seat, serial); }
    void WaylandSurface::request_resize(uint32_t serial, uint32_t edge) { if (m_xdg_toplevel && m_seat) xdg_toplevel_resize(m_xdg_toplevel, m_seat, serial, edge); }
    void WaylandSurface::set_min_size(int w, int h) { if (m_xdg_toplevel) xdg_toplevel_set_min_size(m_xdg_toplevel, w, h); }
    void WaylandSurface::set_max_size(int w, int h) { if (m_xdg_toplevel) xdg_toplevel_set_max_size(m_xdg_toplevel, w, h); }
    void WaylandSurface::request_maximize() { if (m_xdg_toplevel) { xdg_toplevel_set_maximized(m_xdg_toplevel); wl_surface_commit(m_surface); } }
    void WaylandSurface::request_minimize() { if (m_xdg_toplevel) { xdg_toplevel_set_minimized(m_xdg_toplevel); wl_surface_commit(m_surface); } }
    void WaylandSurface::request_restore() { if (m_xdg_toplevel) { xdg_toplevel_unset_maximized(m_xdg_toplevel); wl_surface_commit(m_surface); } }
    void WaylandSurface::request_fullscreen() { if (m_xdg_toplevel) { xdg_toplevel_set_fullscreen(m_xdg_toplevel, nullptr); wl_surface_commit(m_surface); } }
    void WaylandSurface::request_unfullscreen() { if (m_xdg_toplevel) { xdg_toplevel_unset_fullscreen(m_xdg_toplevel); wl_surface_commit(m_surface); } }

    void WaylandSurface::request_activation_token(std::function<void(const std::string &)> callback, uint32_t serial)
    {
        if (!m_activation) { if (callback) callback(""); return; }
        uint32_t use_serial = (serial != 0) ? serial : m_last_serial;
        struct ActivationData { std::function<void(const std::string &)> callback; xdg_activation_token_v1 *token_obj; };
        auto *data = new ActivationData{callback, xdg_activation_v1_get_activation_token(m_activation)};
        static const xdg_activation_token_v1_listener listener = {
            .done = [](void *d, xdg_activation_token_v1 *obj, const char *token) {
                auto *ad = static_cast<ActivationData *>(d);
                if (ad->callback) ad->callback(token ? token : "");
                xdg_activation_token_v1_destroy(obj); delete ad;
            }};
        xdg_activation_token_v1_add_listener(data->token_obj, &listener, data);
        xdg_activation_token_v1_set_serial(data->token_obj, use_serial, m_seat);
        xdg_activation_token_v1_set_surface(data->token_obj, m_surface);
        xdg_activation_token_v1_set_app_id(data->token_obj, m_role == Role::XdgToplevel ? "org.horizon.dock" : m_layer_namespace.c_str());
        xdg_activation_token_v1_commit(data->token_obj);
    }

    void WaylandSurface::activate(const std::string &token) { if (m_activation && !token.empty()) xdg_activation_v1_activate(m_activation, token.c_str(), m_surface); }

    void WaylandSurface::set_cursor(CursorType type) {
        if (!pointer() || !m_cursor_theme || !m_cursor_surface) return;
        const char *name = (type == CursorType::Pointer) ? "hand2" : "left_ptr";
        struct wl_cursor *cursor = wl_cursor_theme_get_cursor(m_cursor_theme, name);
        if (!cursor) return;
        struct wl_cursor_image *image = cursor->images[0];
        wl_pointer_set_cursor(pointer(), m_last_serial, m_cursor_surface, image->hotspot_x, image->hotspot_y);
        wl_surface_attach(m_cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
        wl_surface_damage(m_cursor_surface, 0, 0, image->width, image->height);
        wl_surface_commit(m_cursor_surface);
    }

    void WaylandSurface::activate_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) {
        if (handle && m_foreign_toplevels.count(handle) && m_seat) {
            zwlr_foreign_toplevel_handle_v1_unset_minimized(handle);
            zwlr_foreign_toplevel_handle_v1_activate(handle, m_seat);
        }
    }
    void WaylandSurface::minimize_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) { if (handle && m_foreign_toplevels.count(handle)) zwlr_foreign_toplevel_handle_v1_set_minimized(handle); }
    void WaylandSurface::toggle_fullscreen_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) { if (handle && m_foreign_toplevels.count(handle)) zwlr_foreign_toplevel_handle_v1_set_fullscreen(handle, nullptr); }
    void WaylandSurface::close_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle) { if (handle && m_foreign_toplevels.count(handle)) zwlr_foreign_toplevel_handle_v1_close(handle); }

    void WaylandSurface::update_blur_region() {
        if (!m_surface || !m_blur_enabled || !m_background_effect_manager) return;
        if (!m_background_effect_surface) m_background_effect_surface = ext_background_effect_manager_v1_get_background_effect(m_background_effect_manager, m_surface);
        struct wl_region *region = wl_compositor_create_region(m_compositor);
        wl_region_add(region, 0, 0, m_width, m_height);
        ext_background_effect_surface_v1_set_blur_region(m_background_effect_surface, region);
        wl_region_destroy(region);
        wl_surface_commit(m_surface);
    }

    void WaylandSurface::set_blur(bool enabled) {
        if (m_blur_enabled == enabled) return;
        m_blur_enabled = enabled;
        if (!m_surface) return;
        if (enabled) update_blur_region();
        else if (m_background_effect_surface) { ext_background_effect_surface_v1_destroy(m_background_effect_surface); m_background_effect_surface = nullptr; }
        wl_surface_commit(m_surface);
    }

    static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
        xdg_wm_base_pong(xdg_wm_base, serial);
    }

    static const struct xdg_wm_base_listener xdg_wm_base_listener_impl = {
        .ping = xdg_wm_base_ping,
    };

    // --- Global Handlers ---
    void registry_global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
        auto *ws = static_cast<WaylandSurface *>(data);
        if (strcmp(interface, "wl_compositor") == 0) ws->m_compositor = (wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 4);
        else if (strcmp(interface, "wl_shm") == 0) ws->m_shm = (wl_shm*)wl_registry_bind(registry, id, &wl_shm_interface, 1);
        else if (strcmp(interface, "xdg_wm_base") == 0) {
            ws->m_xdg_wm_base = (xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
            xdg_wm_base_add_listener(ws->m_xdg_wm_base, &xdg_wm_base_listener_impl, ws);
        }
        else if (strcmp(interface, "zwlr_layer_shell_v1") == 0) ws->m_layer_shell = (zwlr_layer_shell_v1*)wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1);
        else if (strcmp(interface, "wl_seat") == 0) { 
            ws->m_seat = (wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 1); 
            wl_seat_add_listener(ws->m_seat, &g_seat_listener, ws); 
            if (ws->m_data_device_manager && !ws->m_data_device) {
                ws->m_data_device = wl_data_device_manager_get_data_device(ws->m_data_device_manager, ws->m_seat);
            }
        }

        else if (strcmp(interface, "xdg_activation_v1") == 0) ws->m_activation = (xdg_activation_v1*)wl_registry_bind(registry, id, &xdg_activation_v1_interface, 1);
        else if (strcmp(interface, "zwlr_foreign_toplevel_manager_v1") == 0) {
            ws->m_foreign_toplevel_manager = (zwlr_foreign_toplevel_manager_v1*)wl_registry_bind(registry, id, &zwlr_foreign_toplevel_manager_v1_interface, 3);
            zwlr_foreign_toplevel_manager_v1_add_listener(ws->m_foreign_toplevel_manager, &foreign_toplevel_manager_listener, ws);
        }
        else if (strcmp(interface, "wl_output") == 0) {
            struct wl_output *o = (wl_output*)wl_registry_bind(registry, id, &wl_output_interface, std::min(version, 4u));
            ws->m_outputs.push_back(o);
            wl_output_add_listener(o, &output_listener, ws);
            
            WaylandSurface::MonitorDetail detail;
            detail.output = o;
            detail.x = 0; detail.y = 0; detail.width = 0; detail.height = 0;
            ws->m_monitor_details.push_back(detail);
        }
        else if (strcmp(interface, ext_background_effect_manager_v1_interface.name) == 0) {
            ws->m_background_effect_manager = (ext_background_effect_manager_v1*)wl_registry_bind(registry, id, &ext_background_effect_manager_v1_interface, 1);
        }
        else if (strcmp(interface, "wl_data_device_manager") == 0) {
            ws->m_data_device_manager = (wl_data_device_manager*)wl_registry_bind(registry, id, &wl_data_device_manager_interface, 3);
            if (ws->m_seat) {
                ws->m_data_device = wl_data_device_manager_get_data_device(ws->m_data_device_manager, ws->m_seat);
            }
        }
    }


    // --- Foreign Toplevel Callbacks ---
    void foreign_toplevel_handle_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, const char *title) { static_cast<WaylandSurface *>(data)->m_foreign_toplevels[handle].title = title ? title : ""; }
    void foreign_toplevel_handle_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, const char *app_id) { static_cast<WaylandSurface *>(data)->m_foreign_toplevels[handle].app_id = app_id ? app_id : ""; }
    void foreign_toplevel_handle_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle, struct wl_array *state) {
        auto *ws = static_cast<WaylandSurface *>(data);
        bool min = false, act = false;
        uint32_t *states = (uint32_t*)state->data;
        for (size_t i = 0; i < state->size / sizeof(uint32_t); ++i) {
            if (states[i] == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED) min = true;
            if (states[i] == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) act = true;
        }
        ws->m_foreign_toplevels[handle].minimized = min;
        ws->m_foreign_toplevels[handle].active = act;
    }
    void foreign_toplevel_handle_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *) { if (static_cast<WaylandSurface *>(data)->listener()) static_cast<WaylandSurface *>(data)->listener()->on_foreign_toplevel_event(); }
    void foreign_toplevel_handle_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *handle) {
        auto *ws = static_cast<WaylandSurface *>(data);
        ws->m_foreign_toplevels.erase(handle);
        zwlr_foreign_toplevel_handle_v1_destroy(handle);
        if (ws->listener()) ws->listener()->on_foreign_toplevel_event();
    }
    void foreign_toplevel_handle_output_enter(void *, struct zwlr_foreign_toplevel_handle_v1 *, struct wl_output *) {}
    void foreign_toplevel_handle_output_leave(void *, struct zwlr_foreign_toplevel_handle_v1 *, struct wl_output *) {}
    void foreign_toplevel_handle_parent(void *, struct zwlr_foreign_toplevel_handle_v1 *, struct zwlr_foreign_toplevel_handle_v1 *) {}
    void foreign_toplevel_manager_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *, struct zwlr_foreign_toplevel_handle_v1 *handle) {
        auto *ws = static_cast<WaylandSurface *>(data);
        ws->m_foreign_toplevels[handle].handle = handle;
        zwlr_foreign_toplevel_handle_v1_add_listener(handle, &foreign_toplevel_handle_listener, ws);
    }

    // --- Input Handlers ---
    static void seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps) {
        auto *ws = static_cast<WaylandSurface *>(data);
        if (caps & WL_SEAT_CAPABILITY_POINTER) { ws->set_wl_pointer(wl_seat_get_pointer(seat)); wl_pointer_add_listener(ws->pointer(), &g_pointer_listener, ws); }
        if (caps & WL_SEAT_CAPABILITY_KEYBOARD) { ws->set_wl_keyboard(wl_seat_get_keyboard(seat)); wl_keyboard_add_listener(ws->keyboard(), &g_keyboard_listener, ws); }
    }
    static void seat_handle_name(void *, wl_seat *, const char *) {}

    static void pointer_handle_enter(void *data, wl_pointer *, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
        if (!surface) return;
        auto *entered_ws = static_cast<WaylandSurface *>(wl_surface_get_user_data(surface));
        if (entered_ws) {
            g_pointer_focus = entered_ws;
            entered_ws->set_pointer_x(wl_fixed_to_double(sx));
            entered_ws->set_pointer_y(wl_fixed_to_double(sy));
            entered_ws->set_last_serial(serial);
            if (entered_ws->listener()) { 
                PointerEvent ev; 
                ev.type = PointerEvent::Type::Enter; 
                ev.x = entered_ws->pointer_x(); 
                ev.y = entered_ws->pointer_y(); 
                ev.serial = serial; 
                entered_ws->listener()->on_pointer_event(ev); 
            }
        }
    }
    static void pointer_handle_leave(void *data, wl_pointer *, uint32_t serial, struct wl_surface *surface) {
        if (!surface) return;
        auto *left_ws = static_cast<WaylandSurface *>(wl_surface_get_user_data(surface));
        if (left_ws) {
            if (g_pointer_focus == left_ws) g_pointer_focus = nullptr;
            left_ws->set_last_serial(serial);
            if (left_ws->listener()) { 
                PointerEvent ev; 
                ev.type = PointerEvent::Type::Leave; 
                ev.serial = serial; 
                left_ws->listener()->on_pointer_event(ev); 
            }
        }
    }
    static void pointer_handle_motion(void *data, wl_pointer *, uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        if (g_pointer_focus) {
            g_pointer_focus->set_pointer_x(wl_fixed_to_double(sx));
            g_pointer_focus->set_pointer_y(wl_fixed_to_double(sy));
            if (g_pointer_focus->listener()) { 
                PointerEvent ev; 
                ev.type = PointerEvent::Type::Move; 
                ev.x = g_pointer_focus->pointer_x(); 
                ev.y = g_pointer_focus->pointer_y(); 
                g_pointer_focus->listener()->on_pointer_event(ev); 
            }
        }
    }
    static void pointer_handle_button(void *data, wl_pointer *, uint32_t serial, uint32_t, uint32_t button, uint32_t state) {
        if (g_pointer_focus) {
            g_pointer_focus->set_last_serial(serial);
            if (g_pointer_focus->listener()) { 
                PointerEvent ev; 
                ev.type = (state == WL_POINTER_BUTTON_STATE_PRESSED) ? PointerEvent::Type::Press : PointerEvent::Type::Release; 
                ev.x = g_pointer_focus->pointer_x(); 
                ev.y = g_pointer_focus->pointer_y(); 
                ev.button = button; 
                ev.serial = serial; 
                g_pointer_focus->listener()->on_pointer_event(ev); 
            }
        }
    }
    static void pointer_handle_axis(void *data, wl_pointer *, uint32_t, uint32_t axis, wl_fixed_t value) {
        auto *ws = static_cast<WaylandSurface *>(data);
        if (ws->listener()) { PointerEvent ev; ev.type = PointerEvent::Type::Scroll; ev.x = ws->pointer_x(); ev.y = ws->pointer_y(); double val = wl_fixed_to_double(value); if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) ev.dy = val; else ev.dx = val; ws->listener()->on_pointer_event(ev); }
    }

    static void keyboard_handle_keymap(void *data, wl_keyboard *, uint32_t format, int32_t fd, uint32_t size) { static_cast<WaylandSurface *>(data)->update_xkb_keymap(format, fd, size); }
    static void keyboard_handle_enter(void *, wl_keyboard *, uint32_t, struct wl_surface *, struct wl_array *) {}
    static void keyboard_handle_leave(void *, wl_keyboard *, uint32_t, struct wl_surface *) {}
    static void keyboard_handle_key(void *data, wl_keyboard *, uint32_t serial, uint32_t, uint32_t key, uint32_t state) {
        auto *ws = static_cast<WaylandSurface *>(data); if (!ws->listener()) return;
        KeyEvent ev; ev.type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? KeyEvent::Type::Press : KeyEvent::Type::Release; ev.key = key; ev.serial = serial;
        ws->process_key(key, state, ev); ws->listener()->on_key_event(ev);
    }
    static void keyboard_handle_modifiers(void *data, wl_keyboard *, uint32_t, uint32_t d, uint32_t la, uint32_t lo, uint32_t g) { static_cast<WaylandSurface *>(data)->update_xkb_modifiers(d, la, lo, g); }
    static void keyboard_handle_repeat_info(void *, wl_keyboard *, int32_t, int32_t) {}

    void WaylandSurface::update_xkb_keymap(uint32_t format, int32_t fd, uint32_t size) {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) { close(fd); return; }
        char *map = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (map == MAP_FAILED) { close(fd); return; }
        struct xkb_keymap *km = xkb_keymap_new_from_string(m_xkb_context, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(map, size); close(fd); if (!km) return;
        struct xkb_state *st = xkb_state_new(km); if (!st) { xkb_keymap_unref(km); return; }
        if (m_xkb_state) xkb_state_unref(m_xkb_state); if (m_xkb_keymap) xkb_keymap_unref(m_xkb_keymap);
        m_xkb_keymap = km; m_xkb_state = st;
    }
    void WaylandSurface::update_xkb_modifiers(uint32_t d, uint32_t la, uint32_t lo, uint32_t g) { if (m_xkb_state) xkb_state_update_mask(m_xkb_state, d, la, lo, 0, 0, g); }
    void WaylandSurface::process_key(uint32_t key, uint32_t state, KeyEvent &ev) {
        if (!m_xkb_state) return;
        xkb_keycode_t kc = key + 8; ev.keysym = xkb_state_key_get_one_sym(m_xkb_state, kc);
        if (state == WL_KEYBOARD_KEY_STATE_PRESSED) { char buf[64]; int sz = xkb_state_key_get_utf8(m_xkb_state, kc, buf, sizeof(buf)); if (sz > 0) ev.text = std::string(buf, sz); }
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE)) ev.modifiers |= 0x1;
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE)) ev.modifiers |= 0x2;
        if (xkb_state_mod_name_is_active(m_xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE)) ev.modifiers |= 0x4;
    }

    // --- Output Handlers Implementation ---
    void output_handle_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
                                      int32_t physical_width, int32_t physical_height, int32_t subpixel,
                                      const char *make, const char *model, int32_t transform)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        for (auto &d : ws->m_monitor_details)
        {
            if (d.output == wl_output)
            {
                d.x = x;
                d.y = y;
                break;
            }
        }
    }

    void output_handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
                                  int32_t width, int32_t height, int32_t refresh)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        for (auto &d : ws->m_monitor_details)
        {
            if (d.output == wl_output)
            {
                WaylandSurface::MonitorModeInfo mode;
                mode.width = width;
                mode.height = height;
                mode.refresh = refresh;
                mode.current = (flags & WL_OUTPUT_MODE_CURRENT);
                mode.preferred = (flags & WL_OUTPUT_MODE_PREFERRED);
                
                bool found = false;
                for (auto &m : d.modes) {
                    if (m.width == width && m.height == height && m.refresh == refresh) {
                        m.current = mode.current;
                        m.preferred = mode.preferred;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    d.modes.push_back(mode);
                }

                if (mode.current)
                {
                    d.width = width;
                    d.height = height;
                }
                break;
            }
        }
    }

    void output_handle_done(void *data, struct wl_output *wl_output)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        LOG_INFO << "[SURFACE] Monitor info done for output " << wl_output;
        ws->when_monitor_update.run(wl_output);
    }

    void output_handle_scale(void *data, struct wl_output *wl_output, int32_t factor)
    {
        // Currently not used in MonitorDetail
    }

    void output_handle_name(void *data, struct wl_output *wl_output, const char *name)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        for (auto &d : ws->m_monitor_details)
        {
            if (d.output == wl_output)
            {
                d.name = name ? name : "";
                break;
            }
        }
    }

    void output_handle_description(void *data, struct wl_output *wl_output, const char *description)
    {
        auto *ws = static_cast<WaylandSurface *>(data);
        for (auto &d : ws->m_monitor_details)
        {
            if (d.output == wl_output)
            {
                d.description = description ? description : "";
                break;
            }
        }
    }

} // namespace horizon
