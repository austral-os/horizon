#pragma once
#include <horizon/Message.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace horizon
{
    /**
     * @brief Thread-safe registry of Message objects.
     *        Renamed from DialogManager for generality.
     */
    class MessageManager
    {
    public:
        void add_message(std::unique_ptr<Message> message)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages[message->id()] = std::move(message);
        }

        Message *get_message(const std::string &id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_messages.find(id);
            if (it != m_messages.end())
            {
                return it->second.get();
            }
            return nullptr;
        }

        void remove_message(const std::string &id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.erase(id);
        }

    private:
        std::map<std::string, std::unique_ptr<Message>> m_messages;
        std::mutex m_mutex;
    };
} // namespace horizon
