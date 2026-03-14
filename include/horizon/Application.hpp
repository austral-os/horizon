
#include "horizon/WaylandWindow.hpp"
#include <GLES2/gl2.h>
#include <horizon/CompositorAppInterface.hpp>
#include <horizon/WaylandSurface.hpp>
#include <thread>
#include <mutex>
#include <atomic>

#pragma once // Solo se incluye una vez.

namespace horizon
{

    class Application
    {

    public:
        /**
         * @brief Constructs an Application with a window of specified size.
         * @param app_id Unique identifier for the application.
         * @param w Width of the application window.
         * @param h Height of the application window.
         */
        explicit Application(const std::string &app_id, int w = 800, int h = 600);

        /**
         * @brief Destructor. Ensures proper cleanup of resources.
         */
        ~Application();

        // Application copy is disabled to prevent resource management issues.
        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        /**
         * @brief Starts the main application loop.
         * This method blocks until the application is quit.
         */
        void run();

        /**
         * @brief Sets the name of the application.
         */
        void set_name(const std::string &name);

        /**
         * @brief Sets the icon name of the application.
         */
        void set_icon_name(const std::string &icon_name);

        /**
         * @brief Sets the root widget of the main application window.
         */
        void set_root(std::unique_ptr<Widget> root);

        /**
         * @brief Creates a new window.
         */
        WaylandWindow *create_window(int w, int h);

        /**
         * @brief Creates a new dialog window, which is a child of the main window.
         */
        WaylandWindow *create_dialog(WaylandWindow *parent, int w, int h);

    protected:
        /**
         * @brief Protected constructor for derived classes that need custom initialization.
         */
        Application(const std::string &app_id, int w, int h, bool defer_init);

    protected:
        std::string m_app_id;
        std::string m_name;
        std::string m_icon_name;

        struct ManagedWindow {
            std::unique_ptr<WaylandWindow> window;
            WaylandWindow* parent{nullptr};
        };

        /**< The list of managed windows. The first one is considered the main window. */
        std::vector<ManagedWindow> m_managed_windows;

        std::atomic<bool> m_is_running{false};
        std::vector<std::thread> m_window_threads;
        std::mutex m_windows_mutex;
    };
} // namespace horizon