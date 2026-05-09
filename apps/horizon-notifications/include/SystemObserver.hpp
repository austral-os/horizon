#pragma once

#include "NotificationManager.hpp"
#include <horizon/dbusutils/DbusHelper.hpp>
#include <memory>
#include <thread>
#include <atomic>

namespace horizon::notifications
{
    class SystemObserver
    {
    public:
        SystemObserver(NotificationManager &manager);
        ~SystemObserver();

        void start();
        void stop();

    private:
        NotificationManager &m_manager;
        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        std::thread m_thread;
        std::atomic<bool> m_running{false};

        void run();
        void handle_message(DBusMessage *msg);
        
        // Specific handlers
        void handle_upower_event(DBusMessage *msg);
        void handle_nm_event(DBusMessage *msg);
        
        // Cache for states to detect changes
        bool m_last_on_battery = false;
        int m_last_volume = -1;
        int m_last_brightness = -1;
    };
}
