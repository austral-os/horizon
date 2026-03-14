
#include "horizon/HznSurface.hpp"
#include "horizon/WaylandEventListener.hpp"
#include <GLES2/gl2.h>

#include <functional>
#include <horizon/CompositorAppInterface.hpp>
#include <horizon/WaylandSurface.hpp>
#include <map>
#include <memory>

#include <vector>

#pragma once // Solo se incluye una vez.

namespace horizon
{

    class Widget;

    class Menu;
    class ClientMenu;
    class IpcClient;
    class CompositorContext;

    /**
     * @class Application
     * @brief Main application class that orchestrates the Wayland surface, widgets, and event loop.
     *
     * The Application class is responsible for initializing the Wayland surface,
     * managing the widget tree (starting from the root widget), and running the
     * main event loop that dispatches input and system events.
     */
    class Application : public HznSurface
    {

    public:
        /**
         * @brief Constructs an Application with a window of specified size.
         * @param app_id Unique identifier for the application.
         * @param w Width of the application window.
         * @param h Height of the application window.
         */
        explicit Application(const std::string &app_id, int w, int h);

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

        /**
         * @brief Sets the global menu for the application.
         * The menus will be automatically provided to the system when the app gains focus.
         * @param menus A list of root menus.
         */
        void set_global_menu(const std::vector<Menu *> &menus);
        void init_global_menu();

        /**
         * @brief Starts the main application loop.
         * This method blocks until the application is quit.
         */
        void run();

        /**
         * @brief Implementation of the WaylandEventListener pointer event callback.
         * @param event The pointer event details.
         */
        void on_pointer_event(const PointerEvent &event) override;
        void on_key_event(const KeyEvent &event) override;
        void on_modifiers_event(uint32_t modifiers) override;
        void on_resize(int width, int height) override;
        void on_activated(bool active) override;
        void on_foreign_toplevel_event() override;
        virtual void on_close() override;

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

        virtual CompositorAppInterface *compositor_apps()
        {
            return nullptr;
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
        Application(const std::string &app_id, int w, int h, bool defer_init);

    private:
        /**
         * @brief Internal event dispatcher.
         */
        void dispatch_events();

    private:
        /**< The Wayland surface representing the main window. */

        bool m_is_activated = false; /**< Flag indicating if the application is currently active. */

        std::vector<Menu *> m_global_menus;
        std::unique_ptr<Menu> m_app_menu;
        std::shared_ptr<ClientMenu> m_client_menu;

        double m_pointer_x = 0.0; /**< Last known X position of the pointer. */
        double m_pointer_y = 0.0; /**< Last known Y position of the pointer. */

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
        void handle_wheel(const PointerEvent &event);

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

        std::map<size_t, std::function<void(int, int)>> m_on_resize_handlers;

        size_t m_next_app_handler_id{0};

        std::unique_ptr<IpcClient> m_ipc_subscriber;
    };
} // namespace horizon