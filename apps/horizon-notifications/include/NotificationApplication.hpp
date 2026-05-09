#pragma once

#include <horizon/Application.hpp>
#include "NotificationManager.hpp"
#include "NotificationWindow.hpp"
#include "DbusNotificationServer.hpp"
#include "SystemObserver.hpp"
#include <memory>

namespace horizon::notifications
{
    class NotificationApplication : public horizon::Application
    {
    public:
        NotificationApplication();
        virtual ~NotificationApplication() override;

        void run_notifications();

    private:
        NotificationManager m_manager;
        std::unique_ptr<DbusNotificationServer> m_dbus_server;
        std::unique_ptr<SystemObserver> m_system_observer;
    };
}
