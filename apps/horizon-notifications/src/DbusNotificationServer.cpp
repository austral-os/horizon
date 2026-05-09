#include "DbusNotificationServer.hpp"
#include <horizon/Logger.hpp>
#include <iostream>

namespace horizon::notifications
{
    DbusNotificationServer::DbusNotificationServer(NotificationManager &manager)
        : m_manager(manager)
    {
    }

    DbusNotificationServer::~DbusNotificationServer()
    {
        stop();
    }

    void DbusNotificationServer::start()
    {
        m_running = true;
        m_thread = std::thread(&DbusNotificationServer::run, this);
    }

    void DbusNotificationServer::stop()
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void DbusNotificationServer::run()
    {
        try
        {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SESSION);
            
            const char* service_name = "org.freedesktop.Notifications";
            if (getenv("HORIZON_NOTIFICATIONS_TEST")) {
                service_name = "org.horizon.Notifications";
            }

            // Request the name
            DBusError error;
            dbus_error_init(&error);
            int ret = dbus_bus_request_name(m_dbus->get_connection(), 
                                            service_name, 
                                            DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
            
            if (dbus_error_is_set(&error))
            {
                std::cerr << "Failed to request D-Bus name: " << error.message << std::endl;
                dbus_error_free(&error);
                return;
            }
            
            if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
                LOG_INFO << "[DBUS] Registered as " << service_name;
            else
                LOG_INFO << "[DBUS] D-Bus name request result: " << ret << " (already owned by another process: " << service_name << ")";

            while (m_running)
            {
                DBusMessage *msg = m_dbus->pop_message(100);
                if (msg)
                {
                    handle_message(msg);
                    dbus_message_unref(msg);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "D-Bus Server Error: " << e.what() << std::endl;
        }
    }

    void DbusNotificationServer::handle_message(DBusMessage *msg)
    {
        if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "Notify"))
        {
            handle_notify(msg);
        }
        else if (dbus_message_is_method_call(msg, "org.freedesktop.Notifications", "GetCapabilities"))
        {
            DBusMessage *reply = dbus_message_new_method_return(msg);
            DBusMessageIter iter, array_iter;
            dbus_message_iter_init_append(reply, &iter);
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &array_iter);
            
            const char *cap1 = "body";
            const char *cap2 = "icons";
            dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &cap1);
            dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &cap2);
            
            dbus_message_iter_close_container(&iter, &array_iter);
            dbus_connection_send(m_dbus->get_connection(), reply, nullptr);
            dbus_message_unref(reply);
        }
        // Handle other methods like GetServerInformation if needed
    }

    void DbusNotificationServer::handle_notify(DBusMessage *msg)
    {
        DBusMessageIter iter;
        dbus_message_iter_init(msg, &iter);

        const char *app_name, *app_icon, *summary, *body;
        uint32_t replaces_id;
        int32_t expire_timeout;

        // Parse arguments
        dbus_message_iter_get_basic(&iter, &app_name);
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &replaces_id);
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &app_icon);
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &summary);
        dbus_message_iter_next(&iter);
        dbus_message_iter_get_basic(&iter, &body);
        dbus_message_iter_next(&iter);
        
        // Skip actions (as)
        dbus_message_iter_next(&iter);
        // Skip hints (a{sv})
        dbus_message_iter_next(&iter);
        
        dbus_message_iter_get_basic(&iter, &expire_timeout);
        
        LOG_INFO << "[DBUS] Received Notification: " << summary << " (" << app_name << ")";

        uint32_t id = m_manager.add_notification(app_name, app_icon, summary, body, expire_timeout);

        // Reply with the ID
        DBusMessage *reply = dbus_message_new_method_return(msg);
        dbus_message_append_args(reply, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID);
        dbus_connection_send(m_dbus->get_connection(), reply, nullptr);
        dbus_message_unref(reply);
    }
}
