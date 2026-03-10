#pragma once

#include "horizon/WaylandEventListener.hpp"
#include "horizon/Widget.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <xkbcommon/xkbcommon.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <wayland-egl.h>

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_surface;
struct xdg_wm_base;
struct xdg_toplevel;
struct xdg_surface;
struct wl_buffer;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_cursor_theme;
struct wl_cursor;
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;
struct wl_output;
struct xdg_activation_v1;
struct xdg_activation_token_v1;
struct zwlr_foreign_toplevel_manager_v1;
struct zwlr_foreign_toplevel_handle_v1;
struct wl_egl_window;

namespace horizon
{
    /**
     * @class WaylandSurface
     * @brief Represents a Wayland surface and manages its associated resources.
     */
    class WaylandSurface
    {
        // Friend declarations for Wayland callbacks
        friend void registry_global(void *, struct wl_registry *, uint32_t, const char *, uint32_t);
        friend void foreign_toplevel_manager_toplevel(void *,
                                                      struct zwlr_foreign_toplevel_manager_v1 *,
                                                      struct zwlr_foreign_toplevel_handle_v1 *);
        friend void foreign_toplevel_handle_title(void *, struct zwlr_foreign_toplevel_handle_v1 *,
                                                  const char *);
        friend void foreign_toplevel_handle_app_id(void *, struct zwlr_foreign_toplevel_handle_v1 *,
                                                   const char *);
        friend void foreign_toplevel_handle_state(void *, struct zwlr_foreign_toplevel_handle_v1 *,
                                                  struct wl_array *);
        friend void foreign_toplevel_handle_closed(void *,
                                                   struct zwlr_foreign_toplevel_handle_v1 *);
        friend void foreign_toplevel_handle_done(void *, struct zwlr_foreign_toplevel_handle_v1 *);

    public:
        enum class Role
        {
            None,
            XdgToplevel,
            LayerShell
        };

        explicit WaylandSurface(int w, int h);
        ~WaylandSurface();

        // Registry setters (called during init)
        void set_wl_compositor(struct wl_compositor *compositor);
        void set_wl_shm(struct wl_shm *shm);
        void set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base);
        void set_zwlr_layer_shell(struct zwlr_layer_shell_v1 *layer_shell);
        void set_wl_seat(struct wl_seat *seat);
        void set_wl_pointer(struct wl_pointer *pointer);
        void set_wl_keyboard(struct wl_keyboard *keyboard);
        void set_xdg_activation(struct xdg_activation_v1 *activation);
        void set_zwlr_foreign_toplevel_manager(struct zwlr_foreign_toplevel_manager_v1 *manager);

        void set_event_listener(WaylandEventListener *listener);

        void set_pointer_x(double x);
        void set_pointer_y(double y);

        // Getters
        struct wl_pointer *pointer() const;
        struct wl_keyboard *keyboard() const;
        struct wl_seat *seat() const;
        struct xdg_wm_base *xdg_wm_base() const;
        struct zwlr_layer_shell_v1 *layer_shell() const;
        struct zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager() const
        {
            return m_foreign_toplevel_manager;
        }
        void *data() const;
        struct wl_surface *surface() const;
        struct wl_buffer *buffer() const;
        struct wl_display *display() const;
        const std::vector<struct wl_output *> &monitors() const
        {
            return m_outputs;
        }
        struct wl_output *get_monitor(size_t index) const
        {
            if (index < m_outputs.size())
                return m_outputs[index];
            return nullptr;
        }

        void move_layer_to_monitor(struct wl_output *output);
        void add_wl_output(struct wl_output *output);
        WaylandEventListener *listener() const;
        bool is_configured() const
        {
            return m_configured;
        }
        double pointer_x() const;
        double pointer_y() const;
        int width() const;
        int height() const;

        /**
         * @brief Initializes the Wayland connection and binds globals.
         */
        void init_display();

        /**
         * @brief Sets up the surface as a standard desktop window.
         */
        void setup_xdg_toplevel(const std::string &title, const std::string &app_id);

        /**
         * @brief Sets up the surface as a layer shell overlay.
         * @param layer The layer to place the surface in (e.g., ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY).
         * @param namespace_id A string identifying the application/role.
         */
        void setup_layer_surface(uint32_t layer, const std::string &namespace_id);

        /**
         * @brief Sets the anchor for a layer surface.
         */
        void set_layer_anchor(uint32_t anchor);

        /**
         * @brief Sets the exclusive zone for a layer surface.
         */
        void set_layer_exclusive_zone(int32_t zone);
        void set_layer_keyboard_interactivity(uint32_t interactivity);
        void set_layer_size(uint32_t width, uint32_t height);

        /**
         * @brief Sets the input region for the surface.
         * Only clicks within this region will be handled by the surface.
         * If the region is empty, the surface will be click-through.
         */
        void set_input_region(int x, int y, int w, int h);

        /**
         * @brief Clears the input region, making the entire surface click-through.
         */
        void clear_input_region();

        /**
         * @brief Compatibility init (defaults to xdg_toplevel).
         */
        void init();

        void set_last_serial(uint32_t serial);
        void free();
        void resize_buffer(int width, int height);
        void commit();

        void request_move(uint32_t serial);
        void request_resize(uint32_t serial, uint32_t edge);

        void update_xkb_keymap(uint32_t format, int32_t fd, uint32_t size);
        void update_xkb_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
                                  uint32_t mods_locked, uint32_t group);
        void process_key(uint32_t key, uint32_t state, KeyEvent &ev);

        uint32_t last_serial() const;

        // Toplevel specific
        void request_maximize();
        void request_minimize();
        void request_restore();
        void request_fullscreen();
        void request_unfullscreen();
        bool is_maximized() const;
        bool is_fullscreen() const;
        void request_activation_token(std::function<void(const std::string &)> callback,
                                      uint32_t serial = 0);
        void activate(const std::string &token);

        void set_cursor(CursorType type);

        // Foreign toplevel management (for Dock)
        void restore_foreign_app(const std::string &app_id);

        struct xkb_state *xkb_state() const
        {
            return m_xkb_state;
        }

        EGLDisplay egl_display() const
        {
            return m_egl_display;
        }
        EGLSurface egl_surface() const
        {
            return m_egl_surface;
        }
        EGLContext egl_context() const
        {
            return m_egl_context;
        }
        void swap_buffers();

    private:
        int m_width;
        int m_height;
        Role m_role = Role::None;

        struct wl_display *m_display = nullptr;
        struct wl_registry *m_registry = nullptr;
        struct wl_compositor *m_compositor = nullptr;
        struct wl_shm *m_shm = nullptr;
        struct xdg_wm_base *m_xdg_wm_base = nullptr;
        struct zwlr_layer_shell_v1 *m_layer_shell = nullptr;
        struct xdg_activation_v1 *m_activation = nullptr;
        struct zwlr_foreign_toplevel_manager_v1 *m_foreign_toplevel_manager = nullptr;

        // EGL Objects
        EGLDisplay m_egl_display = EGL_NO_DISPLAY;
        EGLConfig m_egl_config;
        EGLContext m_egl_context = EGL_NO_CONTEXT;
        EGLSurface m_egl_surface = EGL_NO_SURFACE;
        struct wl_egl_window *m_egl_window = nullptr;

        void init_egl();

        // Role specific objects
        struct xdg_surface *m_xdg_surface = nullptr;
        struct xdg_toplevel *m_xdg_toplevel = nullptr;
        struct zwlr_layer_surface_v1 *m_layer_surface = nullptr;

        void *m_data = nullptr;
        struct wl_surface *m_surface = nullptr;
        struct wl_buffer *m_buffer = nullptr;

        WaylandEventListener *m_listener = nullptr;

        struct wl_seat *m_seat = nullptr;
        struct wl_pointer *m_pointer = nullptr;
        struct wl_keyboard *m_keyboard = nullptr;
        std::vector<struct wl_output *> m_outputs;

        uint32_t m_last_serial = 0;
        double m_pointer_x = 0;
        double m_pointer_y = 0;
        bool m_is_maximized = false;
        bool m_is_fullscreen = false;
        bool m_is_activated = false;

        struct wl_cursor_theme *m_cursor_theme = nullptr;
        struct wl_surface *m_cursor_surface = nullptr;
        CursorType m_current_cursor_type = CursorType::Default;

        struct xkb_context *m_xkb_context = nullptr;
        struct xkb_keymap *m_xkb_keymap = nullptr;
        struct xkb_state *m_xkb_state = nullptr;

        // Foreign toplevel tracking
        struct ForeignToplevel
        {
            struct zwlr_foreign_toplevel_handle_v1 *handle;
            std::string title;
            std::string app_id;
            bool minimized = false;
            bool active = false;
        };
        std::map<struct zwlr_foreign_toplevel_handle_v1 *, ForeignToplevel> m_foreign_toplevels;

        // Layer Shell State tracking
        uint32_t m_layer_num = 0;
        std::string m_layer_namespace;
        uint32_t m_anchor = 0;
        int32_t m_exclusive_zone = 0;
        uint32_t m_interactivity = 0;
        bool m_configured = false;
    };
} // namespace horizon