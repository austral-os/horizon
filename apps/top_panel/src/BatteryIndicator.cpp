#include "BatteryIndicator.hpp"
#include <dbus/dbus.h>
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Notification.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <cmath>
#include <sstream>

using namespace horizon;
using namespace horizon::dbusutils;

BatteryIndicator::BatteryIndicator() : ITopPanelWidget()
{
    // Icon
    auto icon = std::make_unique<Icon>();
    m_icon = icon.get();
    m_icon->set_icon_size(24);
    add_child(std::move(icon));

    // Label for percentage
    auto label = std::make_unique<Label>("");
    m_label = label.get();
    m_label->set_vertical_alignment(VerticalAlignment::Middle);
    add_child(std::move(label));

    set_spacing(4); // Space between icon and label

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
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path='/org/freedesktop/UPower/devices/DisplayDevice'", &err);
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.UPower',member='DeviceChanged'", &err);

    // Initial check
    BatteryStatus initial_status = calculate_current_info();
    if (application()) {
        application()->post_task([this, initial_status]() { 
            this->update_ui(initial_status.icon_name, initial_status.percentage, initial_status.is_present); 
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
            application()->post_task([this, status]() { 
                this->update_ui(status.icon_name, status.percentage, status.is_present); 
            });
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
    const std::string path = "/org/freedesktop/UPower/devices/DisplayDevice";
    const std::string iface = "org.freedesktop.UPower.Device";

    auto is_present_var = local_dbus->get_property("org.freedesktop.UPower", path, iface, "IsPresent");
    info.is_present = std::holds_alternative<bool>(is_present_var) ? std::get<bool>(is_present_var) : false;

    if (info.is_present) {
        auto percentage_var = local_dbus->get_property("org.freedesktop.UPower", path, iface, "Percentage");
        info.percentage = std::holds_alternative<double>(percentage_var) ? std::get<double>(percentage_var) : 0.0;

        auto icon_var = local_dbus->get_property("org.freedesktop.UPower", path, iface, "IconName");
        info.icon_name = std::holds_alternative<std::string>(icon_var) ? std::get<std::string>(icon_var) : "battery-missing-symbolic";
    } else {
        // Fallback or check if it's really missing
        info.icon_name = "battery-missing-symbolic";
        info.percentage = 0.0;
    }

    return info;
}

void BatteryIndicator::update_ui(const std::string &icon_name, double percentage, bool is_present)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    
    m_current_icon_name = icon_name;
    m_current_percentage = percentage;
    m_current_is_present = is_present;

    if (!is_present) {
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
    if (!is_visible()) return 0;
    // Icon (24) + Margin (4) + Label width
    return 24 + 4 + m_label->preferred_width();
}
