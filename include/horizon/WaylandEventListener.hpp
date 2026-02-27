#pragma once

#include <cstdint>
#include <string>

namespace horizon
{

    /**
     * @struct PointerEvent
     * @brief Encapsulates details about a mouse or touch pointer event.
     */
    struct PointerEvent
    {
        /**
         * @enum Type
         * @brief Enumerates the different types of pointer interactions.
         */
        enum class Type
        {
            Move,    /**< Pointer position changed. */
            Press,   /**< A button was pressed. */
            Release, /**< A button was released. */
            Enter,   /**< Pointer entered the surface. */
            Leave,   /**< Pointer left the surface. */
            Scroll   /**< Scroll wheel or touch scroll event. */
        };

        Type type{Type::Move}; /**< The type of the pointer event. */
        double x{0.0};         /**< X coordinate relative to the surface top-left. */
        double y{0.0};         /**< Y coordinate relative to the surface top-left. */
        uint32_t button{
            0}; /**< The button identifier (e.g., BTN_LEFT from <linux/input-event-codes.h>). */
        uint32_t serial{0}; /**< The Wayland serial of the event. */
    };

    /**
     * @struct KeyEvent
     * @brief Encapsulates details about a physical keyboard key event.
     */
    struct KeyEvent
    {
        /**
         * @enum Type
         * @brief The state of the key (pressed or released).
         */
        enum class Type
        {
            Press,  /**< Key was pressed. */
            Release /**< Key was released. */
        };

        Type type{Type::Press}; /**< The type of the key event. */
        uint32_t key{0};        /**< The hardware key code. */
        uint32_t modifiers{0};  /**< Modifier keys bitmask. */
        uint32_t keysym{0};     /**< The XKB keysym. */
        std::string text;       /**< The UTF-8 text associated with the key. */
    };

    /**
     * @class WaylandEventListener
     * @brief Interface for objects that wish to receive input events from a Wayland surface.
     *
     * Implement this interface and register it with a WaylandSurface to get
     * notified about user interaction.
     */
    class WaylandEventListener
    {
    public:
        /**
         * @brief Virtual destructor to ensure proper cleanup of derived classes.
         */
        virtual ~WaylandEventListener() = default;

        /**
         * @brief Called when a pointer event occurs on the associated surface.
         * @param event The pointer event details.
         */
        virtual void on_pointer_event(const PointerEvent &event) = 0;

        /**
         * @brief Called when a keyboard event occurs while the surface has focus.
         * @param event The key event details.
         */
        virtual void on_key_event(const KeyEvent &event) = 0;

        /**
         * @brief Called when the surface is resized by the compositor.
         * @param width New width of the surface.
         * @param height New height of the surface.
         */
        virtual void on_resize(int width, int height) = 0;
    };
} // namespace horizon