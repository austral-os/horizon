#include <algorithm>
#include <horizon/SignalManager.hpp>

namespace horizon
{
    size_t SignalManager::connect(const std::string &signal, Callback callback)
    {
        size_t id = m_next_id++;
        m_handlers.push_back({id, signal, std::move(callback)});
        return id;
    }

    void SignalManager::disconnect(size_t id)
    {
        m_handlers.erase(std::remove_if(m_handlers.begin(), m_handlers.end(),
                                        [id](const Handler &h) { return h.id == id; }),
                         m_handlers.end());
    }

    void SignalManager::emit(const std::string &signal, SignalContext &context)
    {
        context.signal = signal;

        auto current_handlers = m_handlers;
        for (auto &handler : current_handlers)
        {
            if (context.stop_propagation)
                break;

            if (handler.signal == signal && handler.callback)
            {
                handler.callback(context);
            }
        }
    }

    void SignalManager::emit(const std::string &signal, void *sender)
    {
        SignalContext context;
        context.sender = sender;
        emit(signal, context);
    }

} // namespace horizon
