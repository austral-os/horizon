#include "NotificationManager.hpp"
#include <horizon/Application.hpp>
#include <algorithm>

namespace horizon::notifications
{
    NotificationManager::NotificationManager()
    {
    }

    NotificationManager::~NotificationManager()
    {
        stop_watching();
    }

    void NotificationManager::initialize(Application* app)
    {
        m_app = app;
        
        char *home = getenv("HOME");
        std::string config_path = home ? std::string(home) + "/.config/horizon/notifications.json" : "notifications.json";
        
        m_config = std::make_unique<ConfigManager>(config_path);
        load_config();

        start_watching(config_path);
    }

    void NotificationManager::on_file_changed()
    {
        load_config();
    }

    void NotificationManager::post_watcher_task(std::function<void()> task)
    {
        if (m_app) m_app->post_task(task);
    }

    void NotificationManager::load_config()
    {
        if (m_config->load()) {
            m_enabled = m_config->get_value("general", "enabled", true).get<bool>();
        } else {
            m_enabled = true; // Default
        }
    }

    uint32_t NotificationManager::add_notification(const std::string &app_name,
                                                   const std::string &icon_name,
                                                   const std::string &summary,
                                                   const std::string &body,
                                                   int timeout)
    {
        if (!m_enabled) {
            return 0;
        }

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
