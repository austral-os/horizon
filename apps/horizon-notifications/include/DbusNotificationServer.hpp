#pragma once

#include "NotificationManager.hpp"
#include <horizon/dbusutils/DbusHelper.hpp>
#include <dbus/dbus.h>
#include <thread>
#include <atomic>

namespace horizon::notifications
{
    class DbusNotificationServer
    {
    public:
        DbusNotificationServer(NotificationManager &manager);
        ~DbusNotificationServer();

        void start();
        void stop();

    private:
        NotificationManager &m_manager;
        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        std::thread m_thread;
        std::atomic<bool> m_running{false};

        void run();
        void handle_message(DBusMessage *msg);
        void handle_notify(DBusMessage *msg);
    };
}
