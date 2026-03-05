#include <algorithm>
#include <horizon/EventsManager.hpp>

namespace horizon
{
    size_t EventsManager::connect(Callback callback)
    {
        size_t id = m_next_id++;
        m_handlers.push_back({id, std::move(callback)});
        return id;
    }

    void EventsManager::disconnect(size_t id)
    {
        m_handlers.erase(std::remove_if(m_handlers.begin(), m_handlers.end(),
                                        [id](const Handler &h) { return h.id == id; }),
                         m_handlers.end());
    }

    void EventsManager::run(EventContext &context)
    {
        // We use a copy of the handlers or careful iteration to
        // allow disconnection during execution if needed.
        auto current_handlers = m_handlers;
        for (auto &handler : current_handlers)
        {
            if (handler.callback)
            {
                handler.callback(context);
            }
        }
    }
} // namespace horizon
