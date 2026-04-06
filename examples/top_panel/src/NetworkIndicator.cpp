#include "NetworkIndicator.hpp"
#include <dbus/dbus.h>
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <iostream>

using namespace horizon;
using namespace horizon::dbusutils;

NetworkIndicator::NetworkIndicator() : ITopPanelWidget()
{
    auto icon = std::make_unique<Icon>();
    m_icon = icon.get();
    m_icon->set_icon_size(24);
    m_icon->set_icon_name("nm-signal-0-symbolic"); // Default
    m_current_icon_name = "nm-signal-0-symbolic";

    add_child(std::move(icon));

    try
    {
        m_dbus = std::make_unique<DbusHelper>(DBUS_BUS_SYSTEM);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "NetworkIndicator: Failed to init D-Bus: " << e.what();
    }

    when_application_load.connect(
        [this](EventContext &)
        {
            update_status();

            // Start monitor thread
            m_stop_monitor = false;
            m_monitor_thread = std::thread(&NetworkIndicator::monitor_loop, this);
        });
}

NetworkIndicator::~NetworkIndicator()
{
    m_stop_monitor = true;
    if (m_monitor_thread.joinable())
    {
        m_monitor_thread.join();
    }
}

void NetworkIndicator::monitor_loop()
{
    DBusError err;
    dbus_error_init(&err);

    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err))
    {
        dbus_error_free(&err);
        return;
    }

    // Match NM signals
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.NetworkManager'", &err);
    dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.DBus.Properties'", &err);

    while (!m_stop_monitor)
    {
        dbus_connection_read_write_dispatch(conn, 500); // Wait 500ms

        DBusMessage *msg = dbus_connection_pop_message(conn);
        if (!msg)
        {
            continue;
        }

        bool should_update = false;
        if (dbus_message_is_signal(msg, "org.freedesktop.NetworkManager", "StateChanged") ||
            dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged"))
        {
            should_update = true;
        }

        dbus_message_unref(msg);

        if (should_update && application())
        {
            application()->post_task([this]() { this->update_status(); });
        }
    }

    dbus_connection_unref(conn);
}

void NetworkIndicator::update_status()
{
    if (!m_dbus)
    {
        LOG_ERROR << "NetworkIndicator: D-Bus helper is NULL!";
        return;
    }

    LOG_INFO << "NetworkIndicator: Updating status...";

    std::string icon_name = "network-offline";

    // Get ActiveConnections
    auto active_conns_var =
        m_dbus->get_property("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
                             "org.freedesktop.NetworkManager", "ActiveConnections");

    if (std::holds_alternative<std::vector<std::string>>(active_conns_var))
    {
        auto paths = std::get<std::vector<std::string>>(active_conns_var);
        LOG_INFO << "NetworkIndicator: Found " << paths.size() << " active connections in NM";

        std::string best_wifi_icon = "";
        bool ethernet_found = false;

        for (const auto &path : paths)
        {
            // 1. Check if the connection is actually ACTIVATED (State 2)
            auto state_var =
                m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                     "org.freedesktop.NetworkManager.Connection.Active", "State");
            uint32_t state =
                std::holds_alternative<uint32_t>(state_var) ? std::get<uint32_t>(state_var) : 0;

            auto type_var =
                m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                     "org.freedesktop.NetworkManager.Connection.Active", "Type");

            if (std::holds_alternative<std::string>(type_var))
            {
                std::string type = std::get<std::string>(type_var);
                LOG_INFO << "NetworkIndicator: Found active connection type: " << type
                         << " (State: " << state << ")";

                if (state != 2)
                    continue; // Only process fully activated connections

                if (type == "802-3-ethernet" || type == "ethernet")
                {
                    // Check if the device is physical Ethernet (DeviceType 1)
                    auto devices_var = m_dbus->get_property(
                        "org.freedesktop.NetworkManager", path,
                        "org.freedesktop.NetworkManager.Connection.Active", "Devices");
                    if (std::holds_alternative<std::vector<std::string>>(devices_var))
                    {
                        auto devs = std::get<std::vector<std::string>>(devices_var);
                        if (!devs.empty())
                        {
                            auto dev_type_var = m_dbus->get_property(
                                "org.freedesktop.NetworkManager", devs[0],
                                "org.freedesktop.NetworkManager.Device", "DeviceType");
                            if (std::holds_alternative<uint32_t>(dev_type_var) &&
                                std::get<uint32_t>(dev_type_var) == 1)
                            {
                                LOG_INFO << "NetworkIndicator: Physical ethernet device found at: "
                                         << devs[0];
                                ethernet_found = true;
                            }
                        }
                    }
                }
                else if (type == "802-11-wireless" || type == "wireless")
                {
                    auto devices_var = m_dbus->get_property(
                        "org.freedesktop.NetworkManager", path,
                        "org.freedesktop.NetworkManager.Connection.Active", "Devices");
                    if (std::holds_alternative<std::vector<std::string>>(devices_var))
                    {
                        auto devs = std::get<std::vector<std::string>>(devices_var);
                        if (!devs.empty())
                        {
                            LOG_INFO << "NetworkIndicator: Wireless device found at: " << devs[0];
                            int strength = get_wifi_signal_strength(devs[0]);
                            best_wifi_icon = get_icon_for_wifi(strength);
                        }
                    }
                }
            }
        }

        // Prioritization: If we have WiFi, show it. Otherwise show Ethernet if found.
        if (!best_wifi_icon.empty())
        {
            icon_name = best_wifi_icon;
        }
        else if (ethernet_found)
        {
            icon_name = "network-wired";
        }
    }

    if (m_current_icon_name != icon_name)
    {
        m_current_icon_name = icon_name;
        m_icon->set_icon_name(icon_name);
        invalidate();
    }
}

int NetworkIndicator::get_wifi_signal_strength(const std::string &device_path)
{
    auto active_ap_var =
        m_dbus->get_property("org.freedesktop.NetworkManager", device_path,
                             "org.freedesktop.NetworkManager.Device.Wireless", "ActiveAccessPoint");

    if (std::holds_alternative<std::string>(active_ap_var))
    {
        std::string ap_path = std::get<std::string>(active_ap_var);
        if (ap_path != "/" && !ap_path.empty())
        {
            auto strength_var =
                m_dbus->get_property("org.freedesktop.NetworkManager", ap_path,
                                     "org.freedesktop.NetworkManager.AccessPoint", "Strength");
            if (std::holds_alternative<uint32_t>(strength_var))
            {
                return (int)std::get<uint32_t>(strength_var);
            }
        }
    }
    return 0;
}

std::string NetworkIndicator::get_icon_for_wifi(int strength)
{
    if (strength > 85)
        return "nm-signal-100";
    if (strength > 65)
        return "nm-signal-75";
    if (strength > 40)
        return "nm-signal-50";
    if (strength > 15)
        return "nm-signal-25";
    return "nm-signal-0";
}

int NetworkIndicator::preferred_width() const
{
    return m_icon->preferred_width();
}
