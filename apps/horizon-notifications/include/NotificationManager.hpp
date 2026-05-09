#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace horizon::notifications
{
    struct NotificationData
    {
        uint32_t id;
        std::string app_name;
        std::string icon_name;
        std::string summary;
        std::string body;
        int timeout;
    };

    class NotificationManager
    {
    public:
        NotificationManager();
        ~NotificationManager();

        uint32_t add_notification(const std::string &app_name,
                                  const std::string &icon_name,
                                  const std::string &summary,
                                  const std::string &body,
                                  int timeout);

        void remove_notification(uint32_t id);
        
        const std::vector<NotificationData>& active_notifications() const { return m_notifications; }

        void set_on_changed_callback(std::function<void()> callback) { m_on_changed = callback; }

    private:
        std::vector<NotificationData> m_notifications;
        uint32_t m_next_id = 1;
        std::function<void()> m_on_changed;
        
        const size_t MAX_NOTIFICATIONS = 5;
    };
}
