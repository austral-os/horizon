#include "NotificationApplication.hpp"
#include <horizon/I18n.hpp>

namespace horizon::notifications
{
    NotificationApplication::NotificationApplication()
        : Application("horizon-notifications", 1, 1, true, true)
    {
        about_manager().set_app_title("Horizon Notifications");
        about_manager().set_app_description("Notification daemon for Horizon");
        about_manager().set_app_version("1.0.0");
        about_manager().set_app_icon("dialog-information");
        
        // Load translations
        i18n().load_app_locales("horizon-notifications");

        m_manager.initialize(this);
        
        auto win = std::make_unique<NotificationWindow>(m_manager);
        m_managed_windows.push_back({std::move(win), nullptr, {}});
        
        m_dbus_server = std::make_unique<DbusNotificationServer>(m_manager);
        m_system_observer = std::make_unique<SystemObserver>(m_manager);
    }

    NotificationApplication::~NotificationApplication()
    {
        m_dbus_server->stop();
        m_system_observer->stop();
    }

    void NotificationApplication::run_notifications()
    {
        m_dbus_server->start();
        m_system_observer->start();
        
        run(); // Call base run() which is void
    }
}
