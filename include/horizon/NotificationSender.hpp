#ifndef HORIZON_NOTIFICATION_SENDER_HPP
#define HORIZON_NOTIFICATION_SENDER_HPP

#include <string>

namespace horizon
{
    /**
     * @brief Utility class to send desktop notifications via D-Bus.
     * 
     * This class implements the client side of the org.freedesktop.Notifications standard.
     */
    class NotificationSender
    {
    public:
        /**
         * @brief Sends a notification.
         * 
         * @param summary The summary/title of the notification.
         * @param body The body text of the notification.
         * @param icon_name The name of the icon to display (from theme).
         * @param timeout The timeout in milliseconds (0 for never expire, -1 for default).
         */
        static void send(const std::string &summary,
                         const std::string &body,
                         const std::string &icon_name = "dialog-information",
                         int timeout = 5000);
    };
} // namespace horizon

#endif // HORIZON_NOTIFICATION_SENDER_HPP
