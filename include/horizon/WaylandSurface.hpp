#pragma once

#include "horizon/WaylandEventListener.hpp"
#include "horizon/Widget.hpp"
#include <cstdint>
#include <xkbcommon/xkbcommon.h>

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;
struct wl_surface;
struct xdg_wm_base;
struct xdg_toplevel;
struct wl_buffer;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_cursor_theme;
struct wl_cursor;

namespace horizon
{
    /**
     * @class WaylandSurface
     * @brief Represents a Wayland surface and manages its associated resources.
     *
     * This class handles the connection to the Wayland display, creation of the
     * compositor surface, management of input devices (pointer, keyboard), and
     * integration with XDG Shell for window management.
     */
    class WaylandSurface
    {
    public:
        /**
         * @brief Constructs a WaylandSurface with the specified width and height.
         * @param w width of the surface.
         * @param h height of the surface.
         */
        explicit WaylandSurface(int w, int h);

        /**
         * @brief Destructor. Cleans up all Wayland resources.
         */
        ~WaylandSurface();

        /**
         * @brief Sets the Wayland compositor object.
         * @param compositor Pointer to the wl_compositor.
         */
        void set_wl_compositor(struct wl_compositor *compositor);

        /**
         * @brief Sets the Wayland SHM object.
         * @param shm Pointer to the wl_shm.
         */
        void set_wl_shm(struct wl_shm *shm);

        /**
         * @brief Sets the XDG WM Base object.
         * @param xdg_wm_base Pointer to the xdg_wm_base.
         */
        void set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base);

        /**
         * @brief Sets the Wayland seat object.
         * @param seat Pointer to the wl_seat.
         */
        void set_wl_seat(struct wl_seat *seat);

        /**
         * @brief Sets the Wayland pointer object.
         * @param pointer Pointer to the wl_pointer.
         */
        void set_wl_pointer(struct wl_pointer *pointer);

        /**
         * @brief Sets the Wayland keyboard object.
         * @param keyboard Pointer to the wl_keyboard.
         */
        void set_wl_keyboard(struct wl_keyboard *keyboard);

        /**
         * @brief Registers an event listener to receive Wayland events.
         * @param listener Pointer to the WaylandEventListener implementation.
         */
        void set_event_listener(WaylandEventListener *listener);

        /**
         * @brief Sets the current pointer X coordinate.
         * @param x X coordinate in surface-local coordinates.
         */
        void set_pointer_x(double x);

        /**
         * @brief Sets the current pointer Y coordinate.
         * @param y Y coordinate in surface-local coordinates.
         */
        void set_pointer_y(double y);

        /**
         * @brief Gets the Wayland pointer object.
         * @return Pointer to wl_pointer.
         */
        struct wl_pointer *pointer() const;

        /**
         * @brief Gets the Wayland keyboard object.
         * @return Pointer to wl_keyboard.
         */
        struct wl_keyboard *keyboard() const;

        /**
         * @brief Gets the Wayland seat object.
         * @return Pointer to wl_seat.
         */
        struct wl_seat *seat() const;

        /**
         * @brief Gets the XDG WM Base object.
         * @return Pointer to xdg_wm_base.
         */
        struct xdg_wm_base *xdg_wm_base() const;

        /**
         * @brief Gets a pointer to the surface's pixel data.
         * @return Pointer to the raw buffer data.
         */
        void *data() const;

        /**
         * @brief Gets the underlying Wayland surface object.
         * @return Pointer to wl_surface.
         */
        struct wl_surface *surface() const;

        /**
         * @brief Gets the current Wayland buffer object.
         * @return Pointer to wl_buffer.
         */
        struct wl_buffer *buffer() const;

        /**
         * @brief Gets the Wayland display object.
         * @return Pointer to wl_display.
         */
        struct wl_display *display() const;

        /**
         * @return Pointer to the registered event listener.
         */
        WaylandEventListener *listener() const;

        /**
         * @return Current pointer X coordinate.
         */
        double pointer_x() const;

        /**
         * @return Current pointer Y coordinate.
         */
        double pointer_y() const;

        /**
         * @return Width of the surface.
         */
        int width() const;

        /**
         * @return Height of the surface.
         */
        int height() const;

        /**
         * @brief Initializes the Wayland connection and creates resources.
         * This includes connecting to the display, binding globals, and creating the surface.
         */
        void init();

        /**
         * @brief Sets the last received serial.
         * @param serial The serial number.
         */
        void set_last_serial(uint32_t serial);

        /**
         * @brief Frees all allocated Wayland resources.
         */
        void free();

        /**
         * @brief Resizes the underlying buffer and shared memory.
         * @param width New width.
         * @param height New height.
         */
        void resize_buffer(int width, int height);

        /**
         * @brief Requests the compositor to start a window move (dragging) operation.
         * @param serial The serial of the button press event that initiated the move.
         */
        void request_move(uint32_t serial);

        /**
         * @brief Requests the compositor to start an interactive window resize operation.
         * @param serial The serial of the button press event that initiated the resize.
         * @param edge The edge or corner to resize from (see xdg_toplevel_resize_edge).
         */
        void request_resize(uint32_t serial, uint32_t edge);

        /**
         * @brief Updates the XKB keymap from the compositor.
         */
        void update_xkb_keymap(uint32_t format, int32_t fd, uint32_t size);

        /**
         * @brief Updates the XKB modifiers state.
         */
        void update_xkb_modifiers(uint32_t mods_depressed, uint32_t mods_latched,
                                  uint32_t mods_locked, uint32_t group);

        /**
         * @brief Processes a raw key event using XKB.
         */
        void process_key(uint32_t key, uint32_t state, KeyEvent &ev);

        /**
         * @return The serial of the last handled input event.
         */
        uint32_t last_serial() const;

        /**
         * @brief Requests the compositor to maximize the window.
         */
        void request_maximize();

        /**
         * @brief Requests the compositor to minimize the window.
         */
        void request_minimize();

        /**
         * @brief Requests the compositor to restore the window from a maximized state.
         */
        void request_restore();

        /**
         * @return True if the window is currently maximized.
         */
        bool is_maximized() const;

        /**
         * @brief Sets the cursor image based on the specified type.
         * @param type The desired cursor type.
         */
        void set_cursor(CursorType type);

        /**
         * @return The current XKB state.
         */
        struct xkb_state *xkb_state() const
        {
            return m_xkb_state;
        }

    private:
        int m_width;  /**< Width of the surface in pixels. */
        int m_height; /**< Height of the surface in pixels. */

        struct wl_display *m_display = nullptr; /**< The connection to the Wayland server. */
        struct wl_registry *m_registry =
            nullptr; /**< The Wayland registry for globals exploration. */
        struct wl_compositor *m_compositor = nullptr; /**< The compositor global. */
        struct wl_shm *m_shm = nullptr;               /**< The shared memory global for buffers. */
        struct xdg_wm_base *m_xdg_wm_base = nullptr;  /**< The XDG window management base global. */
        struct xdg_toplevel *m_xdg_toplevel = nullptr; /**< The XDG toplevel global. */

        void *m_data = nullptr;                 /**< Pointer to the shared memory data. */
        struct wl_surface *m_surface = nullptr; /**< The Wayland surface object. */
        struct wl_buffer *m_buffer = nullptr;   /**< The current buffer attached to the surface. */

        WaylandEventListener *m_listener = nullptr; /**< The interface for forwarding events. */

        struct wl_seat *m_seat =
            nullptr; /**< The seat object representing a group of input devices. */
        struct wl_pointer *m_pointer = nullptr;   /**< The pointer input device. */
        struct wl_keyboard *m_keyboard = nullptr; /**< The keyboard input device. */

        uint32_t m_last_serial = 0; /**< The last received event serial number. */
        double m_pointer_x = 0;     /**< Last known pointer X position. */
        double m_pointer_y = 0;     /**< Last known pointer Y position. */
        bool m_is_maximized = false;

        struct wl_cursor_theme *m_cursor_theme = nullptr;
        struct wl_surface *m_cursor_surface = nullptr;
        CursorType m_current_cursor_type = CursorType::Default;

        struct xkb_context *m_xkb_context = nullptr;
        struct xkb_keymap *m_xkb_keymap = nullptr;
        struct xkb_state *m_xkb_state = nullptr;
    };
}; // namespace horizon