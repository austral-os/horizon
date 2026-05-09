#include <horizon/NotificationSender.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <dbus/dbus.h>

namespace horizon
{
    void NotificationSender::send(const std::string &summary,
                                  const std::string &body,
                                  const std::string &icon_name,
                                  int timeout)
    {
        try
        {
            dbusutils::DbusHelper dbus(DBUS_BUS_SESSION);
            DBusConnection *conn = dbus.get_connection();

            const char* service_name = "org.freedesktop.Notifications";
            if (getenv("HORIZON_NOTIFICATIONS_TEST")) {
                service_name = "org.horizon.Notifications";
            }

            DBusMessage *msg = dbus_message_new_method_call(service_name,
                                                            "/org/freedesktop/Notifications",
                                                            "org.freedesktop.Notifications",
                                                            "Notify");

            if (!msg)
                return;

            const char *app_name = "Horizon";
            uint32_t replaces_id = 0;
            const char *app_icon = icon_name.c_str();
            const char *summary_ptr = summary.c_str();
            const char *body_ptr = body.c_str();
            int32_t expire_timeout = timeout;

            DBusMessageIter iter;
            dbus_message_iter_init_append(msg, &iter);

            // 1. app_name (s)
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &app_name);
            // 2. replaces_id (u)
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &replaces_id);
            // 3. app_icon (s)
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &app_icon);
            // 4. summary (s)
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &summary_ptr);
            // 5. body (s)
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &body_ptr);

            // 6. actions (as) - Empty array for now
            DBusMessageIter actions_iter;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &actions_iter);
            dbus_message_iter_close_container(&iter, &actions_iter);

            // 7. hints (a{sv}) - Empty array for now
            DBusMessageIter hints_iter;
            dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &hints_iter);
            dbus_message_iter_close_container(&iter, &hints_iter);

            // 8. expire_timeout (i)
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &expire_timeout);

            // Send message and don't wait for reply (fire and forget)
            dbus_connection_send(conn, msg, nullptr);
            dbus_connection_flush(conn);
            dbus_message_unref(msg);
        }
        catch (...)
        {
            // Silently fail if D-Bus is not available or other errors occur
            // Notifications should not crash the calling app.
        }
    }
} // namespace horizon
