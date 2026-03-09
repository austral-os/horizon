#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace horizon
{
    /**
     * @brief Base class for all event contexts.
     */
    class EventContext
    {
    public:
        virtual ~EventContext() = default;
        void *sender{nullptr};
        bool stop_propagation{false};
    };

    /**
     * @brief Specialized event contexts.
     */
    class MouseMoveEventContext : public EventContext
    {
    public:
        double x;
        double y;
        uint32_t modifiers{0};
    };

    class MouseButtonEventContext : public EventContext
    {
    public:
        uint32_t button;
        uint32_t modifiers{0};
        uint32_t serial{0};
        double x;
        double y;
    };

    class KeyEventContext : public EventContext
    {
    public:
        uint32_t key;
        uint32_t modifiers;
        uint32_t keysym;
        std::string text;
    };

    class AppEventContext : public EventContext
    {
    public:
        int width{0};
        int height{0};
    };

    class ThemeEventContext : public EventContext
    {
    public:
    };

    class MouseWheelEventContext : public EventContext
    {
    public:
        double dx{0.0};
        double dy{0.0};
        double x{0.0};
        double y{0.0};
        uint32_t modifiers{0};
    };

    /**
     * @class EventsManager
     * @brief Templated manager for event callbacks.
     */
    template <typename EventT> class EventsManager
    {
    public:
        using Callback = std::function<void(EventT &)>;

        /**
         * @brief Connects a callback to the event manager.
         * @param callback The function to execute when run() is called.
         * @return A unique subscription ID.
         */
        size_t connect(Callback callback)
        {
            size_t id = m_next_id++;
            m_handlers.push_back({id, std::move(callback)});
            return id;
        }

        /**
         * @brief Disconnects a callback using its ID.
         * @param id The ID returned by connect().
         */
        void disconnect(size_t id)
        {
            for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it)
            {
                if (it->id == id)
                {
                    m_handlers.erase(it);
                    break;
                }
            }
        }

        /**
         * @brief Executes all registered callbacks with the provided context.
         * @param context The event context to pass to handlers.
         */
        void run(EventT &context)
        {
            // We use a copy of the handlers to allow disconnection during execution.
            auto current_handlers = m_handlers;
            for (auto &handler : current_handlers)
            {
                if (handler.callback)
                {
                    handler.callback(context);
                    if (context.stop_propagation)
                    {
                        break;
                    }
                }
            }
        }

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
