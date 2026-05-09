#include "NotificationManager.hpp"
#include <algorithm>

namespace horizon::notifications
{
    NotificationManager::NotificationManager()
    {
    }

    NotificationManager::~NotificationManager()
    {
    }

    uint32_t NotificationManager::add_notification(const std::string &app_name,
                                                   const std::string &icon_name,
                                                   const std::string &summary,
                                                   const std::string &body,
                                                   int timeout)
    {
        uint32_t id = m_next_id++;
        
        NotificationData data;
        data.id = id;
        data.app_name = app_name;
        data.icon_name = icon_name;
        data.summary = summary;
        data.body = body;
        data.timeout = timeout > 0 ? timeout : 5000; // Default 5s

        // If we reached the limit, remove the oldest one
        if (m_notifications.size() >= MAX_NOTIFICATIONS)
        {
            m_notifications.erase(m_notifications.begin());
        }

        m_notifications.push_back(data);

        if (m_on_changed) m_on_changed();

        return id;
    }

    void NotificationManager::remove_notification(uint32_t id)
    {
        auto it = std::find_if(m_notifications.begin(), m_notifications.end(),
                               [id](const NotificationData &d) { return d.id == id; });
        
        if (it != m_notifications.end())
        {
            m_notifications.erase(it);
            if (m_on_changed) m_on_changed();
        }
    }
}
