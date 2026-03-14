
#include "horizon/WaylandWindow.hpp"
#include <GLES2/gl2.h>
#include <horizon/CompositorAppInterface.hpp>
#include <horizon/WaylandSurface.hpp>

#pragma once // Solo se incluye una vez.

namespace horizon
{

    /**
     * @class Application
     * @brief Main application class that orchestrates the Wayland surface, widgets, and event loop.
     *
     * The Application class is responsible for initializing the Wayland surface,
     * managing the widget tree (starting from the root widget), and running the
     * main event loop that dispatches input and system events.
     */
    class Application
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

    protected:
        /**
         * @brief Protected constructor for derived classes that need custom initialization.
         */
        Application(const std::string &app_id, int w, int h, bool defer_init);

    protected:
        /**< The Wayland surface representing the main window. */
        std::unique_ptr<WaylandWindow> current_window;
    };
} // namespace horizon