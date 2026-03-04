#include "horizon/SignalManager.hpp"
#include "horizon/ThemeManager.hpp"
#include "horizon/WaylandEventListener.hpp"
#include <deque>
#include <functional>
#include <horizon/WaylandSurface.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#pragma once // Solo se incluye una vez.

namespace horizon
{

    class Widget;
    class Menu;
    class ClientMenu;
    class IpcClient;

    /**
     * @class Application
     * @brief Main application class that orchestrates the Wayland surface, widgets, and event loop.
     *
     * The Application class is responsible for initializing the Wayland surface,
     * managing the widget tree (starting from the root widget), and running the
     * main event loop that dispatches input and system events.
     */
    class Application : public WaylandEventListener
    {
    public:
        /**
         * @brief Constructs an Application with a window of specified size.
         * @param w Width of the application window.
         * @param h Height of the application window.
         */
        explicit Application(int w, int h);

        /**
         * @brief Destructor. Ensures proper cleanup of resources.
         */
        ~Application();

        // Application copy is disabled to prevent resource management issues.
        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        /**
         * @brief Move constructor.
         * @param other The application to move from.
         */
        Application(Application &&) noexcept;

        /**
         * @brief Move assignment operator.
         * @param other The application to move from.
         * @return Reference to this application.
         */
        Application &operator=(Application &&) noexcept;

        std::unique_ptr<ThemeManager> theme_manager;
        SignalManager signal_manager;

        EventsManager when_activated;
        EventsManager when_deactivated;
        EventsManager when_close;

        /**
         * @brief Sets the global menu for the application.
         * The menus will be automatically provided to the system when the app gains focus.
         * @param menus A list of root menus.
         */
        void set_global_menu(const std::vector<Menu *> &menus);

        /**
         * @brief Sets the root widget of the application's widget tree.
         * @param root Unique pointer to the new root widget.
         */
        void set_root(std::unique_ptr<Widget> root);

        /**
         * @brief Gets the underlying WaylandSurface.
         * @return Pointer to the WaylandSurface managed by this application.
         */
        WaylandSurface *w_surface() const;

        /**
         * @brief Starts the main application loop.
         * This method blocks until the application is quit.
         */
        void run();

        /**
         * @brief Signals the application to stop its event loop and exit.
         */
        void quit();

        /**
         * @brief Signals the application to wake up its event loop (e.g. from another thread).
         */
        void wakeup();

        /**
         * @brief Invalidates the entire application or a specific widget.
         * @param widget The widget to invalidate. If nullptr, the entire window is repainted.
         */
        void invalidate(Widget *widget = nullptr);

        /**
         * @brief Posts a task to be executed on the main application thread.
         * This method is thread-safe.
         * @param task The function to execute.
         */
        void post_task(std::function<void()> task);

        /**
         * @brief Returns whether this application uses a transparent surface.
         * @return true for OverlayApplication, false for regular applications.
         */
        virtual bool is_transparent_surface() const
        {
            return false;
        }

        /**
         * @brief Requests a window move from the Wayland compositor.
         */
        void request_move();

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
         * @brief Requests the window to enter fullscreen mode.
         */
        void fullscreen();

        /**
         * @brief Requests the window to exit fullscreen mode.
         */
        void unfullscreen();

        /**
         * @return True if the window is in fullscreen mode.
         */
        bool is_fullscreen() const;

        /**
         * @brief Implementation of the WaylandEventListener pointer event callback.
         * @param event The pointer event details.
         */
        void on_pointer_event(const PointerEvent &event) override;
        void on_key_event(const KeyEvent &event) override;
        void on_modifiers_event(uint32_t modifiers) override;
        void on_resize(int width, int height) override;
        void on_activated(bool active) override;
        void on_close() override;

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

        size_t add_timer(int ms, std::function<void()> callback, bool repeat = false);
        void stop_timer(size_t id);

        /**
         * @brief Unregisters a widget from internal application state (e.g. dirty lists, focus).
         * Called when a widget is destroyed.
         */
        void unregister_widget(Widget *widget);

        // --- Application Metadata ---
        void set_app_id(const std::string &id)
        {
            m_app_id = id;
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

        // Modifiers
        enum Modifier
        {
            SHIFT = (1 << 0),
            CTRL = (1 << 1),
            ALT = (1 << 2),
            CAPSLOCK = (1 << 3)
        };

    protected:
        /**
         * @brief Protected constructor for derived classes that need custom initialization.
         */
        Application(int w, int h, bool defer_init);

    private:
        /**
         * @brief Internal event dispatcher.
         */
        void dispatch_events();

        /**
         * @brief Notifies the application manager about lifecycle events.
         * @param type Event type (e.g., "app_started", "app_stopped").
         */
        void notify_app_manager(const std::string &type);
        void notify_window_state(bool minimized);

    private:
        std::unique_ptr<WaylandSurface>
            m_surface;               /**< The Wayland surface representing the main window. */
        bool m_is_running = false;   /**< Flag indicating if the event loop is active. */
        bool m_is_activated = false; /**< Flag indicating if the application is currently active. */

        std::vector<Menu *> m_global_menus;
        std::shared_ptr<ClientMenu> m_client_menu;

        bool m_full_repaint = true; /**< Flag indicating if the entire UI needs re-rendering. */
        std::vector<Widget *> m_dirty_widgets; /**< List of widgets that need re-rendering. */

        int m_wakeup_fd{-1}; /**< File descriptor for waking up the event loop. */

        Widget *m_hovered = nullptr; /**< The widget currently under the mouse pointer. */
        Widget *m_pressed = nullptr; /**< The widget currently being pressed by a mouse button. */
        Widget *m_focused = nullptr; /**< The widget currently having keyboard focus. */

        double m_pointer_x = 0.0;   /**< Last known X position of the pointer. */
        double m_pointer_y = 0.0;   /**< Last known Y position of the pointer. */
        uint32_t m_last_serial = 0; /**< Last received Wayland serial. */

        uint32_t m_resize_edge = 0;       /**< Current edge being hovered for resize. */
        const int m_resize_proximity = 8; /**< Distance to edge to trigger resize. */

        uint32_t m_modifiers{0};

        uint64_t m_blink_last_time{0}; /**< Last time the focused widget was blinked. */
        uint64_t m_last_commit_time{
            0}; /**< Timestamp of last wl_surface_commit (for frame limiter). */

        // Key repeat tracking
        uint32_t m_repeat_key = 0;
        uint64_t m_repeat_delay = 500; // ms
        uint64_t m_repeat_rate = 100;  // ms
        uint64_t m_repeat_start_time = 0;
        uint64_t m_repeat_last_time = 0;
        bool m_is_repeating = false;
        KeyEvent m_repeat_event; /**< Full last key event (keysym, text) used for repeats. */

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

        /**
         * @brief Internal handler for keyboard key press events.
         */
        void handle_key_press(const KeyEvent &event);

        /**
         * @brief Internal handler for keyboard key release events.
         */
        void handle_key_release(const KeyEvent &event);

        // Handler maps
        std::map<size_t, std::function<void()>> m_on_start_handlers;
        std::map<size_t, std::function<void()>> m_on_exit_handlers;
        std::map<size_t, std::function<void(int, int)>> m_on_resize_handlers;
        std::map<size_t, std::function<void(bool)>> m_on_maximize_handlers;
        std::map<size_t, std::function<void()>> m_on_minimize_handlers;

        size_t m_next_app_handler_id{0};

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

        // Application Metadata
        std::string m_app_id{"horizon.app"};
        std::string m_name{"Horizon Application"};
        std::string m_icon_name{"system-run"};
        bool m_show_in_dock{true};
        bool m_show_in_system_tray{false};
        bool m_is_minimized{false};
        bool m_was_maximized_before_minimize{false};

        std::deque<std::function<void()>> m_task_queue;
        std::mutex m_task_mutex;

        // m_root is last: destroyed FIRST so widget dtors can safely call
        // stop_timer/unregister_widget
        std::unique_ptr<Widget> m_root; /**< The root of the UI widget hierarchy. */

        std::unique_ptr<IpcClient> m_ipc_subscriber;
    };
} // namespace horizon