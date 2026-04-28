#include "BatteryIndicator.hpp"
#include <cmath>
#include <dbus/dbus.h>
#include <horizon/Logger.hpp>
#include <horizon/Notification.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <iomanip>
#include <sstream>

using namespace horizon;
using namespace horizon::dbusutils;

BatteryIndicator::BatteryIndicator() : ITopPanelWidget()
{
    // Icon
    auto icon = std::make_unique<Icon>();
    m_icon = icon.get();
    m_icon->set_icon_size(20);
    m_icon->set_fixed_size(24);
    m_icon->set_vertical_alignment(VerticalAlignment::Middle);
    m_icon->set_icon_color(Color(0.2f, 0.2f, 0.2f, 1.0f)); // Emerald green for battery
    add_child(std::move(icon));

    // Label for percentage
    auto label = std::make_unique<Label>("");
    m_label = label.get();
    // m_label->set_font_size(16);
    m_label->set_vertical_alignment(VerticalAlignment::Middle);
    add_child(std::move(label));

    set_spacing(0); // Space between icon and label

    // Initial state (hidden or generic until first check)
    set_visible(false);

    when_application_load.connect(
        [this](EventContext &)
        {
            m_stop_monitor = false;
            m_monitor_thread = std::thread(&BatteryIndicator::monitor_loop, this);
        });
}

BatteryIndicator::~BatteryIndicator()
{
    m_stop_monitor = true;
    if (m_monitor_thread.joinable())
    {
        m_monitor_thread.join();
    }
}

void BatteryIndicator::monitor_loop()
{
    LOG_INFO << "BatteryIndicator: Background monitor thread started.";

    DBusError err;
    dbus_error_init(&err);

    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err))
    {
        LOG_ERROR << "BatteryIndicator Thread: Failed to get D-Bus connection: " << err.message;
        dbus_error_free(&err);
        return;
    }

    // Match UPower signals
    dbus_bus_add_match(conn,
                       "type='signal',interface='org.freedesktop.DBus.Properties',member='"
                       "PropertiesChanged',path='/org/freedesktop/UPower/devices/DisplayDevice'",
                       &err);
    dbus_bus_add_match(
        conn, "type='signal',interface='org.freedesktop.UPower',member='DeviceChanged'", &err);

    // Initial check
    BatteryStatus initial_status = calculate_current_info();
    if (application())
    {
        application()->post_task(
            [this, initial_status]()
            {
                this->update_ui(initial_status.icon_name, initial_status.percentage,
                                initial_status.is_present);
            });
    }

    while (!m_stop_monitor)
    {
        // Wait for signals or timeout (5 seconds)
        dbus_connection_read_write_dispatch(conn, 5000);

        bool triggered = false;
        DBusMessage *msg;
        while ((msg = dbus_connection_pop_message(conn)) != nullptr)
        {
            // We don't strictly need to parse the message, just knowing something changed is enough
            // as we poll the current state anyway.
            triggered = true;
            dbus_message_unref(msg);
        }

        BatteryStatus status = calculate_current_info();

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(m_state_mutex);
            if (status.icon_name != m_current_icon_name ||
                std::abs(status.percentage - m_current_percentage) > 0.1 ||
                status.is_present != m_current_is_present)
            {
                changed = true;
            }
        }

        if (changed && application())
        {
            application()->post_task(
                [this, status]()
                { this->update_ui(status.icon_name, status.percentage, status.is_present); });
        }
    }

    dbus_connection_unref(conn);
}

BatteryIndicator::BatteryStatus BatteryIndicator::calculate_current_info()
{
    static thread_local std::unique_ptr<DbusHelper> local_dbus;
    if (!local_dbus)
    {
        try
        {
            local_dbus = std::make_unique<DbusHelper>(DBUS_BUS_SYSTEM);
        }
        catch (...)
        {
            return {"battery-missing-symbolic", 0.0, false};
        }
    }

    BatteryStatus info;

    // Try to get info from DisplayDevice (composite)
    const std::string display_path = "/org/freedesktop/UPower/devices/DisplayDevice";
    const std::string iface = "org.freedesktop.UPower.Device";

    auto is_present_var =
        local_dbus->get_property("org.freedesktop.UPower", display_path, iface, "IsPresent");
    info.is_present =
        std::holds_alternative<bool>(is_present_var) ? std::get<bool>(is_present_var) : false;

    std::string final_path = display_path;

    if (!info.is_present)
    {
        // Fallback: search for any battery device
        try
        {
            DBusMessage *reply =
                local_dbus->call_method("org.freedesktop.UPower", "/org/freedesktop/UPower",
                                        "org.freedesktop.UPower", "EnumerateDevices");
            if (reply)
            {
                std::vector<std::string> devices = local_dbus->get_object_path_list(reply);
                dbus_message_unref(reply);

                for (const auto &dev_path : devices)
                {
                    auto type_var =
                        local_dbus->get_property("org.freedesktop.UPower", dev_path, iface, "Type");
                    // Type 2 is Battery
                    uint32_t type = 0;
                    if (std::holds_alternative<uint32_t>(type_var))
                        type = std::get<uint32_t>(type_var);
                    else if (std::holds_alternative<uint64_t>(type_var))
                        type = static_cast<uint32_t>(std::get<uint64_t>(type_var));

                    if (type == 2)
                    {
                        auto pres_var = local_dbus->get_property("org.freedesktop.UPower", dev_path,
                                                                 iface, "IsPresent");
                        if (std::holds_alternative<bool>(pres_var) && std::get<bool>(pres_var))
                        {
                            final_path = dev_path;
                            info.is_present = true;
                            break;
                        }
                    }
                }
            }
        }
        catch (...)
        {
            // Ignore errors in fallback
        }
    }

    if (info.is_present)
    {
        auto percentage_var =
            local_dbus->get_property("org.freedesktop.UPower", final_path, iface, "Percentage");
        if (std::holds_alternative<double>(percentage_var))
        {
            info.percentage = std::get<double>(percentage_var);
        }
        else if (std::holds_alternative<uint32_t>(percentage_var))
        {
            info.percentage = static_cast<double>(std::get<uint32_t>(percentage_var));
        }
        else if (std::holds_alternative<uint64_t>(percentage_var))
        {
            info.percentage = static_cast<double>(std::get<uint64_t>(percentage_var));
        }
        else
        {
            info.percentage = 0.0;
        }

        auto icon_var =
            local_dbus->get_property("org.freedesktop.UPower", final_path, iface, "IconName");
        info.icon_name = std::holds_alternative<std::string>(icon_var)
                             ? std::get<std::string>(icon_var)
                             : "battery-missing-symbolic";
    }
    else
    {
        info.icon_name = "battery-missing-symbolic";
        info.percentage = 0.0;
    }

    return info;
}

void BatteryIndicator::update_ui(const std::string &icon_name, double percentage, bool is_present)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);

    LOG_INFO << "BatteryIndicator: Updating UI - Icon: " << icon_name
             << ", Percentage: " << percentage << "%, Present: " << (is_present ? "Yes" : "No");

    m_current_icon_name = icon_name;
    m_current_percentage = percentage;
    m_current_is_present = is_present;

    if (!is_present)
    {
        set_visible(false);
        return;
    }

    set_visible(true);
    m_icon->set_icon_name(icon_name);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(0) << percentage << "%";
    m_label->set_text(ss.str());

    // Update tooltip
    auto tip = std::make_unique<Notification>();
    tip->set_notification(icon_name, "Batería: " + ss.str());
    set_tooltip(std::move(tip));

    invalidate();
}

int BatteryIndicator::preferred_width() const
{
    if (!is_visible())
        return 0;
    // Icon (24) + Spacing (6) + Label width + Safety margin (6)
    return 24 + 6 + m_label->preferred_width() + 6;
}
