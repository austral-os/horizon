#pragma once

#include <cstdint>
#include <string>
#include <vector>

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
        double dx{0.0};     /**< Scroll delta X. */
        double dy{0.0};     /**< Scroll delta Y. */
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
        uint32_t serial{0};     /**< The Wayland serial of the event. */
        std::string text;       /**< The UTF-8 text associated with the key. */
    };

    /**
     * @struct DragDropEvent
     * @brief Encapsulates details about a drag and drop event.
     */
    struct DragDropEvent
    {
        enum class Type
        {
            Enter,
            Motion,
            Leave,
            Drop
        };

        Type type;
        double x{0.0};
        double y{0.0};
        uint32_t serial{0};
        std::vector<std::string> mime_types;
        void *data_offer{nullptr}; // Internal handle to wl_data_offer
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
         * @brief Called when keyboard modifiers (Shift, Ctrl, etc.) change.
         * @param modifiers The new modifiers bitmask.
         */
        virtual void on_modifiers_event(uint32_t modifiers) = 0;

        /**
         * @brief Called when the surface is resized by the compositor.
         * @param width New width of the surface.
         * @param height New height of the surface.
         */
        virtual void on_resize(int width, int height) = 0;

        /**
         * @brief Called when the surface gains or loses activation (focus).
         * @param active True if activated, false if deactivated.
         */
        virtual void on_activated(bool active) = 0;
        
        /**
         * @brief Called when a drag and drop event occurs.
         * @param event The drag and drop event details.
         */
        virtual void on_drag_drop_event(const DragDropEvent &event) {}

        /**
         * @brief Called when a foreign toplevel window (another app) is added, removed or changed.
         */
        virtual void on_foreign_toplevel_event() {}
        
        /**
         * @brief Called when a new clipboard selection offer is received from the compositor.
         */
        virtual void on_clipboard_selection(void *offer) {}

        virtual void on_close() {}
    };
} // namespace horizon