#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "horizon/CompositorAppInterface.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/SignalManager.hpp"
#include "horizon/ThemeManager.hpp"
#include "horizon/WaylandEventListener.hpp"
#include <nlohmann/json.hpp>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

// Forward declarations in global namespace
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct xdg_wm_base;
struct zwlr_layer_shell_v1;
struct xdg_activation_v1;
struct wl_seat;
struct wl_surface;
struct xkb_context;
struct zwlr_foreign_toplevel_manager_v1;
struct ext_foreign_toplevel_list_v1;
struct ext_background_effect_manager_v1;
struct org_kde_kwin_blur_manager;
struct wl_output;
struct xkb_keymap;
struct xkb_state;

namespace horizon
{
    class Widget;
    class Window;
    class Menu;
    class MenuItem;
    class WaylandSurface;

    void registry_global(void *data, struct ::wl_registry *registry, uint32_t id,
                         const char *interface, uint32_t version);

    /**
     * @class Application
     * @brief Main application class that orchestrates the Wayland connection and multiple windows.
     */
    class Application : public WaylandEventListener
    {
        friend class Window;
        friend class WaylandSurface;
        friend class CairoGraphicContext;
        friend void registry_global(void *data, struct ::wl_registry *registry, uint32_t id,
                                    const char *interface, uint32_t version);

    public:
        struct GLDrawCall
        {
            GLuint texture_id;
            float mvp[16];
            float opacity;
            bool use_scissor;
            int scissor_x, scissor_y, scissor_w, scissor_h;
            bool delete_texture;
        };

        enum Modifier
        {
            NONE = 0,
            SHIFT = 1 << 0,
            CTRL = 1 << 1,
            ALT = 1 << 2,
            LOGO = 1 << 3,
        };

        explicit Application(const std::string &app_id, int w = 1280, int h = 720,
                             bool is_service = false);
        virtual ~Application();

        struct Timer
        {
            size_t id;
            int interval;
            std::function<void()> callback;
            bool repeat;
            std::chrono::steady_clock::time_point next_run;
        };

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        std::unique_ptr<ThemeManager> theme_manager;
        SignalManager signal_manager;

        EventsManager<AppEventContext> when_activated;
        EventsManager<AppEventContext> when_deactivated;
        EventsManager<AppEventContext> when_close;
        EventsManager<AppListEventContext> when_foreign_update;

        void set_global_menu(const std::vector<Menu *> &menus);

        void set_root_window(std::unique_ptr<Window> window);
        void set_root(std::unique_ptr<Window> window);
        void register_window(Window *window);
        void unregister_window(Window *window);

        Window *active_window() const
        {
            return m_active_window;
        }
        Window *pointer_window() const
        {
            return m_pointer_window;
        }
        Window *keyboard_window() const
        {
            return m_keyboard_window;
        }

        void run();
        void quit();
        void wakeup();

        void post_task(std::function<void()> task);

        size_t add_timer(int interval_ms, std::function<void()> callback, bool repeat = false);
        void stop_timer(size_t timer_id);

        int width() const
        {
            return m_width;
        }
        int height() const
        {
            return m_height;
        }

        const std::string &app_id() const
        {
            return m_app_id;
        }

        void set_name(const std::string &name)
        {
            m_name = name;
        }
        const std::string &name() const
        {
            return m_name;
        }

        void set_icon_name(const std::string &icon_name)
        {
            m_icon_name = icon_name;
        }
        const std::string &icon_name() const
        {
            return m_icon_name;
        }

        void set_show_in_dock(bool show)
        {
            m_show_in_dock = show;
        }
        bool show_in_dock() const
        {
            return m_show_in_dock;
        }

        void set_show_in_system_tray(bool show)
        {
            m_show_in_system_tray = show;
        }
        bool show_in_system_tray() const
        {
            return m_show_in_system_tray;
        }

        void send_remote_signal(pid_t target_pid, const std::string &signal_name,
                                const std::string &data = "");

        virtual bool is_transparent_surface() const
        {
            return false;
        }
        WaylandSurface *w_surface() const;

        // Wayland globals (shared)
        struct ::wl_display *wl_display() const
        {
            return m_display;
        }
        struct ::wl_compositor *wl_compositor() const
        {
            return m_compositor;
        }
        struct ::wl_shm *wl_shm() const
        {
            return m_shm;
        }
        struct ::xdg_wm_base *xdg_wm_base() const
        {
            return m_xdg_wm_base;
        }
        struct ::zwlr_layer_shell_v1 *wl_layer_shell() const
        {
            return m_layer_shell;
        }
        struct ::wl_seat *wl_seat() const
        {
            return m_seat;
        }
        struct ::xdg_activation_v1 *xdg_activation() const
        {
            return m_activation;
        }
        struct ::zwlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager() const
        {
            return m_foreign_toplevel_manager;
        }
        struct ::ext_foreign_toplevel_list_v1 *ext_foreign_toplevel_list() const
        {
            return m_ext_foreign_toplevel_list;
        }
        struct ::ext_background_effect_manager_v1 *background_effect_manager() const
        {
            return m_background_effect_manager;
        }
        struct ::org_kde_kwin_blur_manager *blur_manager() const
        {
            return m_blur_manager;
        }
        const std::vector<struct ::wl_output *> &outputs() const
        {
            return m_outputs;
        }

        // WaylandEventListener implementation (routing)
        void on_pointer_event(const PointerEvent &event) override;
        void on_key_event(const KeyEvent &event) override;
        void on_modifiers_event(uint32_t modifiers) override;
        void on_resize(int width, int height) override;
        void on_activated(bool active) override;
        void on_foreign_toplevel_event() override;
        void on_close() override;

        void unregister_widget(Widget *w) {}
        void invalidate(Widget *w = nullptr);
        void render_gl_ui(int iteration = 0);

    private:
        void send_app_status(const std::string &type);
        nlohmann::json serialize_menu(Menu *menu);
        nlohmann::json serialize_menu_item(MenuItem *item);
        void ensure_default_menu();
        void dispatch_events();
        void init_wayland();
        void init_egl();

        std::string m_app_id;
        std::string m_name;

        // Shared Wayland State
        struct ::wl_display *m_display = nullptr;
        struct ::wl_registry *m_registry = nullptr;
        struct ::wl_compositor *m_compositor = nullptr;
        struct ::wl_shm *m_shm = nullptr;
        struct ::xdg_wm_base *m_xdg_wm_base = nullptr;
        struct ::zwlr_layer_shell_v1 *m_layer_shell = nullptr;
        struct ::xdg_activation_v1 *m_activation = nullptr;
        struct ::zwlr_foreign_toplevel_manager_v1 *m_foreign_toplevel_manager = nullptr;
        struct ::ext_foreign_toplevel_list_v1 *m_ext_foreign_toplevel_list = nullptr;
        struct ::ext_background_effect_manager_v1 *m_background_effect_manager = nullptr;
        struct ::org_kde_kwin_blur_manager *m_blur_manager = nullptr;
        struct ::wl_seat *m_seat = nullptr;
        std::vector<struct ::wl_output *> m_outputs;

        // EGL Shared context
        EGLDisplay m_egl_display = EGL_NO_DISPLAY;
        EGLConfig m_egl_config;
        EGLContext m_egl_context = EGL_NO_CONTEXT;

        // GLES2 Resources (Shared)
        GLuint m_program = 0;
        GLuint m_v_shader = 0;
        GLuint m_f_shader = 0;
        GLuint m_vbo = 0;
        GLint m_mvp_loc = -1;
        GLint m_opacity_loc = -1;
        GLint m_tex_loc = -1;
        GLint m_pos_loc = -1;
        GLint m_uv_loc = -1;

        // Window Management
        std::unordered_map<struct ::wl_surface *, Window *> m_window_map;
        std::vector<std::unique_ptr<Window>> m_windows;

        Window *m_active_window = nullptr;
        Window *m_pointer_window = nullptr;
        Window *m_keyboard_window = nullptr;

        // Global Caches (moved back to Application as they are sharable resources)
        std::unordered_map<std::string, void *> m_svg_cache;
        std::unordered_map<std::string, void *> m_surface_cache;
        std::recursive_mutex m_cache_mutex;

        int m_wakeup_fd{-1};
        std::deque<std::function<void()>> m_task_queue;
        std::recursive_mutex m_task_mutex;

        std::vector<Timer> m_timers;
        size_t m_next_timer_id{1};
        std::recursive_mutex m_timer_mutex;

        bool m_is_running{false};
        bool m_is_service{false};
        bool m_show_in_dock{true};
        bool m_show_in_system_tray{false};
        bool m_has_registered{false};
        std::string m_icon_name;
        int m_width{1280};
        int m_height{720};

        std::vector<Menu *> m_global_menus;
        std::vector<std::unique_ptr<Menu>> m_default_menus;

        size_t m_clear_menu_timer_id{0};
        // Input state
        uint32_t m_last_serial{0};
        ::xkb_context *m_xkb_context{nullptr};
        ::xkb_keymap *m_xkb_keymap{nullptr};
        ::xkb_state *m_xkb_state{nullptr};
        double m_pointer_x{0};
        double m_pointer_y{0};
    };

} // namespace horizon