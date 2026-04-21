#pragma once

#include "horizon/Clipboard.hpp"
#include "horizon/CompositorAppInterface.hpp"
#include "horizon/CompositorContext.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/SignalManager.hpp"
#include "horizon/ThemeManager.hpp"
#include "horizon/WaylandEventListener.hpp"
#include "horizon/WaylandSurface.hpp"
#include "horizon/Widget.hpp"
#include <atomic>
#include <deque>
#include <functional>
#include <horizon/DialogTypes.hpp>
#include <mutex>

namespace horizon
{

    class GraphicsContext;
    class ClientMenu;
    class IpcClient;
    class PreferencesContent;
    class AboutDialogContent;

    class WaylandWindow : public WaylandEventListener
    {
        friend class CairoGraphicContext;
        class PopupEventListener : public WaylandEventListener
        {
            WaylandWindow *m_window;
            Widget *m_hovered = nullptr;

        public:
            PopupEventListener(WaylandWindow *window) : m_window(window) {}

            void on_pointer_event(const PointerEvent &event) override;
            void on_key_event(const KeyEvent &event) override {}
            void on_modifiers_event(uint32_t modifiers) override {}
            void on_resize(int width, int height) override {}
            void on_activated(bool active) override {}
            void on_close() override;
        };

    public:
        WaylandWindow(std::string app_id = "horizon.app", int w = 800, int h = 600,
                      bool defer_init = false, bool resizable = true, int min_w = -1,
                      int min_h = -1);
        virtual ~WaylandWindow();

        static WaylandWindow *get_active_window()
        {
            return m_active_window;
        }
        void set_clipboard_data(const ClipboardData &data);

        // Modifiers
        enum Modifier
        {
            SHIFT = (1 << 0),
            CTRL = (1 << 1),
            ALT = (1 << 2),
            CAPSLOCK = (1 << 3)
        };

        std::unique_ptr<ThemeManager> theme_manager;

        EventsManager<AppEventContext> when_activated;
        EventsManager<AppEventContext> when_deactivated;
        EventsManager<AppEventContext> when_close;
        EventsManager<AppListEventContext> when_foreign_update;

        SignalManager signal_manager;

        /**
         * @brief Starts the main application loop.
         * This method blocks until the application is quit.
         */
        void run();

        /**
         * @brief Requests the window to be maximized.
         */
        void maximize();

        /**
         * @brief Requests the window to be minimized.
         */
        void minimize();

        /**
         * @brief Requests the window to be restored from maximized state.
         */
        void restore(const std::string &token = "");

        /**
         * @return True if the window is maximized.
         */
        bool is_maximized() const;

        /**
         * @brief Requests a window move from the Wayland compositor.
         */
        void request_move();

        /**
         * @brief Returns whether this application uses a transparent surface.
         * @return true for LayerApplication, false for regular applications.
         */
        virtual bool is_transparent_surface() const
        {
            return true;
        }

        /**
         * @brief Gets the underlying WaylandSurface.
         * @return Pointer to the WaylandSurface managed by this application.
         */
        WaylandSurface *w_surface() const;

        int width() const;
        int height() const;

        size_t add_timer(int ms, std::function<void()> callback, bool repeat = false);
        void stop_timer(size_t id);

        /**
         * @brief Unregisters a widget from internal application state (e.g. dirty lists, focus).
         * Called when a widget is destroyed.
         */
        void unregister_widget(Widget *widget);

        /**
         * @brief Sets the root widget of the application's widget tree.
         * @param root Unique pointer to the new root widget.
         */
        void set_root(std::unique_ptr<Widget> root);
        Widget *root() const
        {
            return m_root.get();
        }
        Menu *get_menu(const std::string &id) const;

        /**
         * @brief Sets the focused widget of the application.
         * @param widget The widget to grab keyboard focus.
         */
        void set_focused_widget(Widget *widget);

        /**
         * @brief Invalidates the entire application or a specific widget.
         * @param widget The widget to invalidate. If nullptr, the entire window is repainted.
         */
        void invalidate(Widget *widget = nullptr);
        void request_clipboard_data(Widget *target, const std::string &mime_type = "text/plain");
        void set_clipboard_owner(Widget *owner);
        std::vector<std::string> get_clipboard_mime_types() const;
        void show_context_menu(Menu *menu, int x = -1, int y = -1, uint32_t serial = 0,
                               Widget *owner = nullptr);
        void close_context_menu(bool emit_signal = true);
        void show_tooltip(Widget *owner, Notification *tooltip);
        void hide_tooltip();
        Widget *tooltip_owner() const
        {
            return m_tooltip_owner;
        }

        /**
         * @brief Signals the application to wake up its event loop (e.g. from another thread).
         */
        void wakeup();

        /**
         * @brief Returns the graphics context for this application.
         */
        virtual GraphicsContext &get_graphics_context() const;

        // Signal emitted when a context menu is closed
        EventsManager<PopupDismissedContext> when_popup_dismissed;

        GLuint gl_program() const
        {
            return m_gl_program;
        }
        GLuint gl_vbo() const
        {
            return m_gl_vbo;
        }

        struct GLDrawCall
        {
            uint32_t texture_id;
            float mvp[16];
            float opacity;
            bool delete_texture;
            bool use_scissor;
            int scissor_x, scissor_y, scissor_w, scissor_h;
            float gradient_start;
            float gradient_end;
        };

        void queue_gl_draw(const GLDrawCall &call) const;

        /**
         * @brief Initializes the window. Should be called from the thread where the event loop will
         * run.
         */
        virtual void initialize();

        /**
         * @brief Signals the application to stop its event loop and exit.
         */
        void quit();

        int screen_x() const
        {
            return m_screen_x;
        }
        int screen_y() const
        {
            return m_screen_y;
        }

        int pointer_x() const
        {
            return (int)m_pointer_x;
        }
        int pointer_y() const
        {
            return (int)m_pointer_y;
        }

        void set_screen_position(int x, int y)
        {
            m_screen_x = x;
            m_screen_y = y;
            if (m_surface)
            {
                m_surface->set_screen_position(x, y);
            }
        }

        /**
         * @brief Returns the current pointer position relative to the monitor's top-left corner.
         */
        widget_position get_global_pointer_position() const;

        void set_resizable(bool resizable);
        bool is_resizable() const
        {
            return m_resizable;
        }

        void set_min_size(int w, int h);
        int min_width() const
        {
            return m_min_width;
        }
        int min_height() const
        {
            return m_min_height;
        }

        /**
         * @brief Posts a task to be executed on the main application thread.
         * This method is thread-safe.
         * @param task The function to execute.
         */
        void post_task(std::function<void()> task);

        /**
         * @return True if the window is minimized.
         */
        bool is_minimized() const;

        /**
         * @return True if the window was maximized before being minimized.
         */
        bool was_maximized_before_minimize() const;

        /**
         * @return True if the window is in fullscreen mode.
         */
        bool is_fullscreen() const;

        /**
         * @brief Requests the window to enter fullscreen mode.
         */
        void fullscreen();

        /**
         * @brief Requests the window to exit fullscreen mode.
         */
        void unfullscreen();
        void set_blur(bool enabled);

        /**
         * @brief Shows an alert dialog.
         */
        void alert(const std::string &message, const std::string &title = "Alert",
                   MessageType type = MessageType::Info);

        /**
         * @brief Shows a confirmation dialog and returns true if accepted.
         */
        bool confirm(const std::string &message, const std::string &title = "Confirm",
                     MessageType type = MessageType::Question);

        /**
         * @brief Sets the application preferences content factory.
         * If set, a "Preferences" item will be automatically added to the global menu.
         * The factory will be invoked each time a preferences dialog is opened.
         */
        using PreferencesFactory = std::function<std::unique_ptr<PreferencesContent>()>;
        void set_preferences_content(PreferencesFactory factory, int width = 500, int height = 400);

        /**
         * @brief Shows the preferences dialog.
         */
        void show_preferences();

        /**
         * @brief Sets the application preferences content factory.
         * If set, a "Preferences" item will be automatically added to the global menu.
         * The factory will be invoked each time a preferences dialog is opened.
         */
        using AboutUsFactory = std::function<std::unique_ptr<AboutDialogContent>()>;
        void set_aboutus_content(AboutUsFactory factory);

        /**
         * @brief Shows the preferences dialog.
         */
        void show_aboutus();

        // --- Application Metadata ---
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

        void on_pointer_event(const PointerEvent &event) override;
        void on_key_event(const KeyEvent &event) override;
        void on_modifiers_event(uint32_t modifiers) override;
        void on_resize(int width, int height) override;
        void on_activated(bool active) override;
        void on_foreign_toplevel_event() override;

        /**
         * @brief Internal handler for pointer movement events.
         */
        void handle_move(const PointerEvent &event);

        /**
         * @brief Internal handler for pointer button press events.
         */
        void handle_press(const PointerEvent &event);

        /**
         * @brief Internal handler for pointer button release events.
         */
        void handle_release(const PointerEvent &event);
        void handle_wheel(const PointerEvent &event);

        /**
         * @brief Internal handler for keyboard key press events.
         */
        void handle_key_press(const KeyEvent &event);

        /**
         * @brief Internal handler for keyboard key release events.
         */
        void handle_key_release(const KeyEvent &event);

        /**
         * @brief Sets the global menu for the application.
         * The menus will be automatically provided to the system when the app gains focus.
         * @param menus A list of root menus.
         */
        void set_global_menu(const std::vector<Menu *> &menus);
        void init_global_menu();

        /**
         * @brief Adds a menu to the window's collection.
         * @param menu The menu to add.
         */
        void add_menu(std::unique_ptr<Menu> menu);

        /**
         * @brief Deletes a menu from the window's collection by its title.
         * @param title The title of the menu to delete.
         */
        void delete_menu(const std::string &title);

        /**
         * @brief Deletes all menus from the window's collection.
         */
        void delete_all_menues();

        /**
         * @brief Sets the application-specific menu.
         * @param menu The menu to set as the application menu.
         */
        void set_app_menu(std::unique_ptr<Menu> menu);

        /**
         * @brief Enables or disables the use of a global system menu for this window.
         * By default, all windows try to register a global menu. Dialogs should usually disable
         * this.
         * @param use True to use global menu, false otherwise.
         */
        void set_use_global_menu(bool use);

        /**
         * @brief Returns the compositor context for this application.
         */
        virtual CompositorContext &get_compositor_context() const;

        // --- Application Events (Multi-Callback) ---
        size_t add_on_start(std::function<void()> handler);
        void remove_on_start(size_t id);

        size_t add_on_exit(std::function<void()> handler);
        void remove_on_exit(size_t id);

        size_t add_on_resize(std::function<void(int, int)> handler);
        void remove_on_resize(size_t id);

        size_t add_on_maximize(std::function<void(bool)> handler);
        void remove_on_maximize(size_t id);

        size_t add_on_minimize(std::function<void()> handler);
        void remove_on_minimize(size_t id);

        /**
         * @brief Sends a signal to another application via the IPC mechanism.
         * @param target_pid The PID of the target application.
         * @param signal The signal name (e.g., "maximize", "close").
         * @param token Optional token for the signal.
         */
        void send_remote_signal(int target_pid, const std::string &signal,
                                const std::string &token = "");

        virtual void on_close() override;

        virtual CompositorAppInterface *compositor_apps()
        {
            return nullptr;
        }

        void apply_fullscreen_isolation(Widget *target);
        void restore_fullscreen_isolation();
        bool detect_fullscreen_support(Widget *root);
        Widget *find_fullscreen_target(Widget *root);

        class Window *find_window_target(Widget *root);

    protected:
        /**
         * @brief Notifies the application manager about lifecycle events.
         * @param type Event type (e.g., "app_started", "app_stopped").
         */
        void notify_app_manager(const std::string &type);
        void notify_window_state(bool minimized);

    private:
        std::atomic<bool> m_is_running{false}; /**< Flag indicating if the event loop is active. */
        bool m_is_activated = false; /**< Flag indicating if the application is currently active. */

        mutable std::mutex m_state_mutex;
        struct Timer
        {
            size_t id;
            int interval_ms;
            uint64_t next_expiry;
            bool repeat;
            std::function<void()> callback;
        };
        std::map<size_t, Timer> m_timers;
        size_t m_next_timer_id{1};

        std::mutex m_task_mutex;
        std::deque<std::function<void()>> m_task_queue;

        uint32_t m_modifiers{0};

        std::vector<Menu *> m_global_menus;
        std::unique_ptr<Menu> m_app_menu;
        std::vector<std::unique_ptr<Menu>> m_menues;
        std::shared_ptr<ClientMenu> m_client_menu;

        // Application Metadata
        std::string m_app_id{"horizon.app"};
        std::string m_name{"Horizon Application"};
        std::string m_icon_name{""};
        bool m_show_in_dock{true};
        bool m_show_in_system_tray{false};

        double m_pointer_x = 0.0; /**< Last known X position of the pointer. */
        double m_pointer_y = 0.0; /**< Last known Y position of the pointer. */

        uint32_t m_last_serial = 0; /**< Last received Wayland serial. */

        std::unique_ptr<WaylandSurface> m_surface;

        int m_screen_x{0};
        int m_screen_y{0};

        std::unique_ptr<Widget> m_root;
        bool m_full_repaint = true; /**< Flag indicating if the entire UI needs re-rendering. */
        bool m_first_frame =
            true; /**< True until the first buffer is committed. Breaking Wayfire deadlock. */
        std::vector<Widget *> m_dirty_widgets; /**< List of widgets that need re-rendering. */
        int m_wakeup_fd{-1};                   /**< File descriptor for waking up the event loop. */

        Widget *m_hovered = nullptr; /**< The widget currently under the mouse pointer. */
        Widget *m_pressed = nullptr; /**< The widget currently being pressed by a mouse button. */
        Widget *m_focused = nullptr; /**< The widget currently having keyboard focus. */

        GLuint m_gl_program{0};
        GLuint m_gl_vbo{0};
        GLuint m_gl_texture{0};
        mutable std::vector<GLDrawCall> m_gl_queue;

        void init_gl_resources();
        void render_gl_ui();
        void render_gl_popup();
        void render_gl_tooltip();

        mutable std::unique_ptr<GraphicsContext> m_gc;
        std::unique_ptr<CompositorContext> m_compositor_context;

        // Image caching
        mutable std::map<std::string, void *> m_svg_cache;
        mutable std::map<std::string, void *> m_surface_cache;

        bool m_is_minimized{false};

        std::map<size_t, std::function<void(int, int)>> m_on_resize_handlers;
        std::map<size_t, std::function<void()>> m_on_exit_handlers;
        std::map<size_t, std::function<void(bool)>> m_on_maximize_handlers;
        std::map<size_t, std::function<void()>> m_on_minimize_handlers;

        bool m_was_maximized_before_minimize{false};

        // Key repeat tracking
        uint32_t m_repeat_key = 0;
        uint64_t m_repeat_delay = 500; // ms
        uint64_t m_repeat_rate = 100;  // ms
        uint64_t m_repeat_start_time = 0;
        uint64_t m_repeat_last_time = 0;
        bool m_is_repeating = false;
        KeyEvent m_repeat_event;

        uint32_t m_resize_edge = 0;       /**< Current edge being hovered for resize. */
        const int m_resize_proximity = 8; /**< Distance to edge to trigger resize. */

        uint64_t m_blink_last_time{0}; /**< Last time the focused widget was blinked. */
        uint64_t m_last_commit_time{
            0}; /**< Timestamp of last wl_surface_commit (for frame limiter). */

        // Handler maps
        std::map<size_t, std::function<void()>> m_on_start_handlers;

        size_t m_next_app_handler_id{0};

        std::unique_ptr<IpcClient> m_ipc_subscriber;

        bool m_resizable = true;

        int m_min_width = -1;
        int m_min_height = -1;

        std::unique_ptr<WaylandSurface> m_popup_surface;
        int m_popup_x{0};
        int m_popup_y{0};
        Menu *m_popup_menu = nullptr;
        std::unique_ptr<PopupEventListener> m_popup_listener;

        std::unique_ptr<WaylandSurface> m_tooltip_surface;
        Notification *m_tooltip_widget = nullptr;
        Widget *m_tooltip_owner = nullptr;

        static WaylandWindow *m_active_window;
        std::unique_ptr<class WaylandClipboardBackend> m_clipboard_backend;

        Widget *m_fullscreen_target{nullptr};
        std::vector<Widget *> m_hidden_by_fullscreen;

        Widget *find_clipboard_target();

        PreferencesFactory m_preferences_factory;
        int m_preferences_width{500};
        int m_preferences_height{400};

        AboutUsFactory m_aboutus_factory;

        bool m_use_global_menu{true};
    };

}; // namespace horizon