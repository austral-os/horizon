#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace horizon
{
    /**
     * @brief Supported event types in the system.
     */
    enum class EventType
    {
        // Mouse Events
        MouseEnter,
        MouseLeave,
        MouseMove,
        MousePress,
        MouseRelease,
        MouseDrag,
        MouseHover,

        // Keyboard Events
        KeyPress,
        KeyRelease,

        // Application Events
        AppStart,
        AppExit,
        AppResize,
        AppMaximize,
        AppMinimize,

        // Theme Events
        ThemeChanged,
    };

    /**
     * @brief Context information passed to every event handler.
     */
    struct EventContext
    {
        void *sender{nullptr};        /**< Pointer to the object that triggered the event. */
        EventType type;               /**< The type of event. */
        uint32_t button{0};           /**< The button that triggered the event. */
        bool stop_propagation{false}; /**< If set to true, subsequent handlers won't be called. */
        void *data{nullptr};          /**< Custom data associated with the event. */
        double eventX{0};
        double eventY{0};
        uint32_t key{0};
        uint32_t modifiers{0};
    };

    /**
     * @class EventsManager
     * @brief Centralized manager for event callbacks.
     */
    class EventsManager
    {
    public:
        using Callback = std::function<void(EventContext &)>;

        /**
         * @brief Connects a callback to the event manager.
         * @param callback The function to execute when run() is called.
         * @return A unique subscription ID.
         */
        size_t connect(Callback callback);

        /**
         * @brief Disconnects a callback using its ID.
         * @param id The ID returned by connect().
         */
        void disconnect(size_t id);

        /**
         * @brief Executes all registered callbacks with the provided context.
         * @param context The event context to pass to handlers.
         */
        void run(EventContext &context);

    private:
        struct Handler
        {
            size_t id;
            Callback callback;
        };

        std::vector<Handler> m_handlers;
        size_t m_next_id{0};
    };

} // namespace horizon
