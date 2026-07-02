#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <type_traits>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>

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
        uint32_t serial{0};
        std::string text;
    };

    class AppEventContext : public EventContext
    {
    public:
        int width{0};
        int height{0};
    };

    class FullscreenEventContext : public EventContext
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

    class MenuBarClickContext : public EventContext
    {
    public:
        class Menu *menu;
        int x;
        int y;
        uint32_t serial{0};
    };

    class PopupDismissedContext : public EventContext
    {
    public:
        uint32_t serial{0};
    };

    class ToggleEventContext : public EventContext
    {
    public:
        bool checked;
    };

    /**
     * @brief Context for drag and drop events.
     */
    class DragEventContext : public MouseMoveEventContext
    {
    public:
        std::vector<std::string> mime_types;
        
        void add_data(const std::string &mime, const std::string &data)
        {
            m_provided_data[mime] = std::vector<uint8_t>(data.begin(), data.end());
            mime_types.push_back(mime);
        }

        void add_data(const std::string &mime, const std::vector<uint8_t> &data)
        {
            m_provided_data[mime] = data;
            mime_types.push_back(mime);
        }

        const std::map<std::string, std::vector<uint8_t>>& provided_data() const { return m_provided_data; }

    private:
        std::map<std::string, std::vector<uint8_t>> m_provided_data;
    };

    class DropEventContext : public DragEventContext
    {
    public:
        std::vector<uint8_t> get_data(const std::string &mime) const
        {
            if (m_data_fetcher) {
                return m_data_fetcher(mime);
            }
            return {};
        }

        std::string get_data_as_string(const std::string &mime) const
        {
            auto data = get_data(mime);
            return std::string(data.begin(), data.end());
        }

        std::function<std::vector<uint8_t>(const std::string &)> m_data_fetcher;
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
            std::lock_guard<std::mutex> lock(m_mutex);
            size_t id = m_next_id++;
            auto handler = std::make_shared<Handler>();
            handler->id = id;
            handler->callback = std::move(callback);
            m_handlers.push_back(handler);
            return id;
        }

        /**
         * @brief Disconnects a callback using its ID.
         * @param id The ID returned by connect().
         */
        void disconnect(size_t id)
        {
            std::shared_ptr<Handler> handler;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it)
                {
                    if ((*it)->id == id)
                    {
                        handler = *it;
                        m_handlers.erase(it);
                        break;
                    }
                }
            }

            if (!handler)
                return;

            deactivate_and_wait(handler);
        }

        /**
         * @brief Disconnects all registered callbacks.
         */
        void disconnect_all()
        {
            std::vector<std::shared_ptr<Handler>> handlers;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                handlers = m_handlers;
                m_handlers.clear();
            }

            for (auto &handler : handlers)
                deactivate_and_wait(handler);
        }

        /**
         * @brief Executes all registered callbacks with the provided context.
         * @param context The event context to pass to handlers.
         */
        void run(EventT &context)
        {
            // We use a copy of the handlers to allow disconnection during execution.
            std::vector<std::shared_ptr<Handler>> current_handlers;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                current_handlers = m_handlers;
            }

            for (auto &handler : current_handlers)
            {
                Callback callback;
                {
                    std::lock_guard<std::mutex> handler_lock(handler->mutex);
                    if (!handler->active || !handler->callback)
                        continue;

                    handler->running++;
                    handler->running_threads[std::this_thread::get_id()]++;
                    callback = handler->callback;
                }

                struct RunningGuard
                {
                    std::shared_ptr<Handler> handler;

                    ~RunningGuard()
                    {
                        std::lock_guard<std::mutex> handler_lock(handler->mutex);
                        handler->running--;

                        auto thread_it = handler->running_threads.find(std::this_thread::get_id());
                        if (thread_it != handler->running_threads.end())
                        {
                            if (thread_it->second <= 1)
                                handler->running_threads.erase(thread_it);
                            else
                                thread_it->second--;
                        }

                        handler->idle.notify_all();
                    }
                } running_guard{handler};

                callback(context);

                if constexpr (std::is_base_of_v<EventContext, EventT>)
                {
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
            bool active{true};
            size_t running{0};
            std::map<std::thread::id, size_t> running_threads;
            std::mutex mutex;
            std::condition_variable idle;
        };

        void deactivate_and_wait(const std::shared_ptr<Handler> &handler)
        {
            std::unique_lock<std::mutex> lock(handler->mutex);
            handler->active = false;

            const auto current_thread = std::this_thread::get_id();
            handler->idle.wait(lock,
                               [&]()
                               {
                                   auto self_it = handler->running_threads.find(current_thread);
                                   size_t self_running = self_it == handler->running_threads.end() ? 0 : self_it->second;
                                   return handler->running == self_running;
                               });

            handler->callback = nullptr;
        }

        std::vector<std::shared_ptr<Handler>> m_handlers;
        size_t m_next_id{0};
        mutable std::mutex m_mutex;
    };

} // namespace horizon
