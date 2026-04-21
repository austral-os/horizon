#pragma once

#include "horizon/WaylandEventListener.hpp"
#include "horizon/Widget.hpp"
#include <cstdint>
#include <map>
#include <protocols/blur-client-protocol.h>
#include <protocols/ext-background-effect-v1-client-protocol.h>
#include <horizon/EventsManager.hpp>
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
struct xdg_popup;
struct xdg_positioner;
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
struct ext_background_effect_manager_v1;
struct ext_background_effect_surface_v1;
struct ext_foreign_toplevel_list_v1;
struct ext_foreign_toplevel_handle_v1;
struct wl_data_device_manager;
struct wl_data_device;
struct wl_data_source;
struct wl_data_offer;
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
        // Foreign toplevel management (zwlr-foreign-toplevel-management-v1)
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
        
        friend void output_handle_geometry(void *, struct wl_output *, int32_t, int32_t, int32_t, int32_t, int32_t, const char *, const char *, int32_t);
        friend void output_handle_mode(void *, struct wl_output *, uint32_t, int32_t, int32_t, int32_t);
        friend void output_handle_done(void *, struct wl_output *);
        friend void output_handle_scale(void *, struct wl_output *, int32_t);
        friend void output_handle_name(void *, struct wl_output *, const char *);
        friend void output_handle_description(void *, struct wl_output *, const char *);

    public:
        enum class Role
        {
            None,
            XdgToplevel,
            LayerShell,
            XdgPopup
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
        void set_ext_background_effect_manager(struct ext_background_effect_manager_v1 *manager);

        void set_event_listener(WaylandEventListener *listener);

        void set_pointer_x(double x);
        void set_pointer_y(double y);

        // Getters
        struct wl_pointer *pointer() const { return m_pointer; }
        struct wl_keyboard *keyboard() const { return m_keyboard; }
        struct wl_seat *seat() const { return m_seat; }
        struct xdg_wm_base *xdg_wm_base() const { return m_xdg_wm_base; }
        struct zwlr_layer_shell_v1 *layer_shell() const { return m_layer_shell; }
        struct zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager() const { return m_foreign_toplevel_manager; }
        struct wl_data_device_manager *data_device_manager() const { return m_data_device_manager; }
        struct wl_data_device *data_device() const { return m_data_device; }
        void *data() const { return m_data; }

        struct wl_surface *surface() const { return m_surface; }
        struct ext_background_effect_manager_v1 *background_effect_manager() const { return m_background_effect_manager; }
        struct wl_buffer *buffer() const { return m_buffer; }
        struct wl_display *display() const { return m_display; }
        struct wl_registry *registry() const { return m_registry; }
        struct wl_compositor *compositor() const { return m_compositor; }
        struct wl_shm *shm() const { return m_shm; }
        const std::vector<struct wl_output *> &monitors() const { return m_outputs; }

        struct MonitorModeInfo
        {
            int width;
            int height;
            int refresh;
            bool current;
            bool preferred;
        };

        struct MonitorDetail
        {
            struct wl_output *output;
            std::string name;
            std::string description;
            int32_t x;
            int32_t y;
            int32_t width;
            int32_t height;
            std::vector<MonitorModeInfo> modes;
        };

        const std::vector<MonitorDetail> &monitor_details() const { return m_monitor_details; }

        EventsManager<struct wl_output*> when_monitor_update;

        struct wl_output *get_monitor(size_t index) const;
        void move_layer_to_monitor(struct wl_output *output);
        void add_wl_output(struct wl_output *output);
        WaylandEventListener *listener() const { return m_listener; }
        bool is_configured() const { return m_configured; }
        double pointer_x() const { return m_pointer_x; }
        double pointer_y() const { return m_pointer_y; }
        int width() const { return m_width; }
        int height() const { return m_height; }
        int monitor_width() const { return m_monitor_width; }
        int monitor_height() const { return m_monitor_height; }
        
        static WaylandSurface *pointer_focus();
        uint32_t anchor() const { return m_anchor; }

        void set_screen_position(int x, int y) { m_screen_x = x; m_screen_y = y; }
        int screen_x() const { return m_screen_x; }
        int screen_y() const { return m_screen_y; }

        Role role() const { return m_role; }

        void init_display();
        void share_connection_from(WaylandSurface *other);
        void setup_xdg_toplevel(const std::string &title, const std::string &app_id);
        void setup_layer_surface(uint32_t layer, const std::string &namespace_id, struct wl_output *output = nullptr);
        uint32_t layer_num() const { return m_layer_num; }
        const std::string &layer_namespace() const { return m_layer_namespace; }
        void setup_xdg_popup(WaylandSurface *parent, int x, int y, int w, int h);
        void set_layer_anchor(uint32_t anchor);
        void set_layer_exclusive_zone(int32_t zone);
        void set_layer_keyboard_interactivity(uint32_t interactivity);
        void set_layer_size(uint32_t width, uint32_t height);
        void set_input_region(int x, int y, int w, int h);
        void clear_input_region();

        void init();
        void set_last_serial(uint32_t serial) { m_last_serial = serial; }
        void free();
        void resize_buffer(int width, int height);
        void commit();
        void set_blur(bool enabled);
        void request_move(uint32_t serial);
        void request_resize(uint32_t serial, uint32_t edge);
        void set_min_size(int w, int h);
        void set_max_size(int w, int h);
        void update_xkb_keymap(uint32_t format, int32_t fd, uint32_t size);
        void update_xkb_modifiers(uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
        void process_key(uint32_t key, uint32_t state, KeyEvent &ev);
        uint32_t last_serial() const { return m_last_serial; }
        void set_seat(struct wl_seat *seat) { m_seat = seat; }

        void request_maximize();
        void request_minimize();
        void request_restore();
        void request_fullscreen();
        void request_unfullscreen();
        bool is_maximized() const { return m_is_maximized; }
        bool is_fullscreen() const { return m_is_fullscreen; }
        void request_activation_token(std::function<void(const std::string &)> callback, uint32_t serial = 0);
        void activate(const std::string &token);

        void set_cursor(CursorType type);

        // Foreign toplevel management (for Dock)
        void activate_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle);
        void minimize_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle);
        void toggle_fullscreen_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle);
        void close_foreign_instance(struct zwlr_foreign_toplevel_handle_v1 *handle);
        
        // Deprecated - replaced by instance versions
        void activate_foreign_app(const std::string &app_id) {}
        void minimize_foreign_app(const std::string &app_id) {}
        void toggle_fullscreen_foreign_app(const std::string &app_id) {}
        void close_foreign_app(const std::string &app_id) {}

        struct ForeignToplevel
        {
            struct zwlr_foreign_toplevel_handle_v1 *handle;
            std::string title;
            std::string app_id;
            bool minimized = false;
            bool active = false;
        };

        const std::map<struct zwlr_foreign_toplevel_handle_v1 *, ForeignToplevel> &get_foreign_toplevels() const { return m_foreign_toplevels; }

        struct xkb_state *xkb_state() const { return m_xkb_state; }
        EGLDisplay egl_display() const { return m_egl_display; }
        EGLSurface egl_surface() const { return m_egl_surface; }
        EGLContext egl_context() const { return m_egl_context; }
        void swap_buffers();
        void update_blur_region();

        struct wl_surface *wl_surface() const { return m_surface; }

    private:
        Role m_role = Role::None;
        int m_width{800}, m_height{600};
        int m_screen_x{0}, m_screen_y{0};
        int m_monitor_width{0}, m_monitor_height{0};
        bool m_is_initialized{false};
        bool m_is_maximized{false};
        bool m_is_minimized{false};
        bool m_is_fullscreen{false};
        bool m_is_activated{false};
        bool m_was_maximized_before_minimize{false};
        bool m_blur_enabled{false};
        uint32_t m_last_serial{0};
        uint32_t m_anchor{0};
        bool m_configured{false};
        bool m_owns_connection{true};

        // Layer shell specifics
        uint32_t m_layer_num{0};
        std::string m_layer_namespace;
        int32_t m_exclusive_zone{0};
        uint32_t m_interactivity{0};
        size_t m_mapped_size{0};

        struct wl_display *m_display{nullptr};
        struct wl_registry *m_registry{nullptr};
        struct wl_compositor *m_compositor{nullptr};
        struct wl_shm *m_shm{nullptr};
        struct wl_surface *m_surface{nullptr};
        struct wl_buffer *m_buffer{nullptr};
        struct wl_seat *m_seat{nullptr};
        struct wl_pointer *m_pointer{nullptr};
        struct wl_keyboard *m_keyboard{nullptr};
        struct wl_output *m_output{nullptr};
        
        struct xdg_wm_base *m_xdg_wm_base{nullptr};
        struct xdg_surface *m_xdg_surface{nullptr};
        struct xdg_toplevel *m_xdg_toplevel{nullptr};
        struct xdg_popup *m_xdg_popup{nullptr};
        struct xdg_positioner *m_xdg_positioner{nullptr};
        struct xdg_activation_v1 *m_activation{nullptr};

        struct zwlr_layer_shell_v1 *m_layer_shell{nullptr};
        struct zwlr_layer_surface_v1 *m_layer_surface{nullptr};
        
        struct wl_egl_window *m_egl_window{nullptr};
        EGLDisplay m_egl_display = EGL_NO_DISPLAY;
        EGLConfig m_egl_config;
        EGLContext m_egl_context = EGL_NO_CONTEXT;
        EGLSurface m_egl_surface = EGL_NO_SURFACE;

        struct xkb_context *m_xkb_context{nullptr};
        struct xkb_keymap *m_xkb_keymap{nullptr};
        struct xkb_state *m_xkb_state{nullptr};

        struct zwlr_foreign_toplevel_manager_v1 *m_foreign_toplevel_manager{nullptr};
        std::map<struct zwlr_foreign_toplevel_handle_v1 *, ForeignToplevel> m_foreign_toplevels;

        struct ext_background_effect_manager_v1 *m_background_effect_manager{nullptr};
        struct ext_background_effect_surface_v1 *m_background_effect_surface{nullptr};
        struct org_kde_kwin_blur_manager *m_blur_manager{nullptr};
        struct org_kde_kwin_blur *m_blur_object{nullptr};

        struct wl_data_device_manager *m_data_device_manager{nullptr};
        struct wl_data_device *m_data_device{nullptr};

        WaylandEventListener *m_listener{nullptr};
        void *m_data{nullptr};


        std::vector<struct wl_output *> m_outputs;
        std::vector<MonitorDetail> m_monitor_details;

        double m_pointer_x{0.0}, m_pointer_y{0.0};

        void init_registry();
        void init_egl();

        struct wl_cursor_theme *m_cursor_theme{nullptr};
        struct wl_surface *m_cursor_surface{nullptr};
        CursorType m_current_cursor_type = CursorType::Default;
    };
} // namespace horizon