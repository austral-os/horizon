#include "horizon/WaylandEventListener.hpp"
#include <horizon/WaylandSurface.hpp>
#include <memory>

#pragma once // Solo se incluye una vez.

namespace horizon
{

    class Widget;

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
        void restore();

        /**
         * @return True if the window is maximized.
         */
        bool is_maximized() const;

        /**
         * @brief Implementation of the WaylandEventListener pointer event callback.
         * @param event The pointer event details.
         */
        void on_pointer_event(const PointerEvent &event) override;

        /**
         * @brief Implementation of the WaylandEventListener keyboard event callback.
         * @param event The keyboard event details.
         */
        void on_key_event(const KeyEvent &event) override;
        void on_resize(int width, int height) override;

    private:
        /**
         * @brief Internal event dispatcher.
         */
        void dispatch_events();

    private:
        std::unique_ptr<WaylandSurface>
            m_surface;             /**< The Wayland surface representing the main window. */
        bool m_is_running = false; /**< Flag indicating if the event loop is active. */
        bool m_dirty = true;       /**< Flag indicating if the UI needs re-rendering. */

        std::unique_ptr<Widget> m_root; /**< The root of the UI widget hierarchy. */

        Widget *m_hovered = nullptr; /**< The widget currently under the mouse pointer. */
        Widget *m_pressed = nullptr; /**< The widget currently being pressed by a mouse button. */

        double m_pointer_x = 0.0;   /**< Last known X position of the pointer. */
        double m_pointer_y = 0.0;   /**< Last known Y position of the pointer. */
        uint32_t m_last_serial = 0; /**< Last received Wayland serial. */

        uint32_t m_resize_edge = 0;       /**< Current edge being hovered for resize. */
        const int m_resize_proximity = 8; /**< Distance to edge to trigger resize. */

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
    };
} // namespace horizon