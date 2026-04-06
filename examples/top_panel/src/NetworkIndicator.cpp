#include "NetworkIndicator.hpp"
#include <dbus/dbus.h>
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>

using namespace horizon;
using namespace horizon::dbusutils;

NetworkIndicator::NetworkIndicator() : ITopPanelWidget()
{
    auto icon = std::make_unique<Icon>();
    m_icon = icon.get();
    m_icon->set_icon_size(24);

    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_current_icon_name = "network-offline";
        m_icon->set_icon_name(m_current_icon_name);
    }

    add_child(std::move(icon));

    when_application_load.connect(
        [this](EventContext &)
        {
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
    LOG_INFO << "NetworkIndicator: Background monitor thread started.";

    // Create a local DbusHelper for this thread to avoid any contention or
    // thread-safety issues with shared connections in a multi-threaded app.
    std::unique_ptr<DbusHelper> thread_dbus;
    try
    {
        thread_dbus = std::make_unique<DbusHelper>(DBUS_BUS_SYSTEM);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "NetworkIndicator Thread: Failed to init D-Bus: " << e.what();
        return;
    }

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

    // Initial check
    std::string initial_icon = calculate_current_icon();
    update_ui(initial_icon);

    while (!m_stop_monitor)
    {
        // Periodically check or wait for signals
        dbus_connection_read_write_dispatch(conn, 2000); // Wait up to 2 seconds

        bool triggered = false;
        DBusMessage *msg;
        while ((msg = dbus_connection_pop_message(conn)) != nullptr)
        {
            if (dbus_message_is_signal(msg, "org.freedesktop.NetworkManager", "StateChanged") ||
                dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged"))
            {
                triggered = true;
            }
            dbus_message_unref(msg);
        }

        // Always calculate if triggered, or if not triggered but some time has passed
        // (the 2000ms timeout in dispatch handles the periodic part).
        std::string new_icon = calculate_current_icon();

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(m_state_mutex);
            if (new_icon != m_current_icon_name)
            {
                changed = true;
                // Don't update m_current_icon_name yet, will happen in update_ui
            }
        }

        if (changed && application())
        {
            application()->post_task([this, new_icon]() { this->update_ui(new_icon); });
        }
    }

    dbus_connection_unref(conn);
}

std::string NetworkIndicator::calculate_current_icon()
{
    // Important: we create a local helper or use one dedicated to this thread
    // to perform purely synchronous, BLOCKING calls in this background thread.
    // This is safe because it doesn't block the UI.

    static thread_local std::unique_ptr<DbusHelper> local_dbus;
    if (!local_dbus)
    {
        try
        {
            local_dbus = std::make_unique<DbusHelper>(DBUS_BUS_SYSTEM);
        }
        catch (...)
        {
            return "network-error";
        }
    }

    std::string icon_name = "network-offline";

    // Get ActiveConnections
    auto active_conns_var = local_dbus->get_property(
        "org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
        "org.freedesktop.NetworkManager", "ActiveConnections");

    if (std::holds_alternative<std::vector<std::string>>(active_conns_var))
    {
        auto paths = std::get<std::vector<std::string>>(active_conns_var);

        std::string best_wifi_icon = "";
        bool ethernet_found = false;

        for (const auto &path : paths)
        {
            auto state_var = local_dbus->get_property(
                "org.freedesktop.NetworkManager", path,
                "org.freedesktop.NetworkManager.Connection.Active", "State");
            uint32_t state =
                std::holds_alternative<uint32_t>(state_var) ? std::get<uint32_t>(state_var) : 0;

            if (state != 2)
                continue; // Only process fully activated connections

            auto type_var = local_dbus->get_property(
                "org.freedesktop.NetworkManager", path,
                "org.freedesktop.NetworkManager.Connection.Active", "Type");

            if (std::holds_alternative<std::string>(type_var))
            {
                std::string type = std::get<std::string>(type_var);

                if (type == "802-3-ethernet" || type == "ethernet")
                {
                    auto devices_var = local_dbus->get_property(
                        "org.freedesktop.NetworkManager", path,
                        "org.freedesktop.NetworkManager.Connection.Active", "Devices");
                    if (std::holds_alternative<std::vector<std::string>>(devices_var))
                    {
                        auto devs = std::get<std::vector<std::string>>(devices_var);
                        if (!devs.empty())
                        {
                            auto dev_type_var = local_dbus->get_property(
                                "org.freedesktop.NetworkManager", devs[0],
                                "org.freedesktop.NetworkManager.Device", "DeviceType");
                            if (std::holds_alternative<uint32_t>(dev_type_var) &&
                                std::get<uint32_t>(dev_type_var) == 1) // NM_DEVICE_TYPE_ETHERNET
                            {
                                ethernet_found = true;
                            }
                        }
                    }
                }
                else if (type == "802-11-wireless" || type == "wireless")
                {
                    auto devices_var = local_dbus->get_property(
                        "org.freedesktop.NetworkManager", path,
                        "org.freedesktop.NetworkManager.Connection.Active", "Devices");
                    if (std::holds_alternative<std::vector<std::string>>(devices_var))
                    {
                        auto devs = std::get<std::vector<std::string>>(devices_var);
                        if (!devs.empty())
                        {
                            int strength = get_wifi_signal_strength(*local_dbus, devs[0]);
                            best_wifi_icon = get_icon_for_wifi(strength);
                        }
                    }
                }
            }
        }

        if (!best_wifi_icon.empty())
            icon_name = best_wifi_icon;
        else if (ethernet_found)
            icon_name = "network-wired";
    }

    return icon_name;
}

void NetworkIndicator::update_ui(const std::string &icon_name)
{
    // This runs on the UI thread
    std::lock_guard<std::mutex> lock(m_state_mutex);
    if (m_current_icon_name != icon_name)
    {
        m_current_icon_name = icon_name;
        m_icon->set_icon_name(icon_name);
        invalidate();
    }
}

int NetworkIndicator::get_wifi_signal_strength(DbusHelper &dbus, const std::string &device_path)
{
    auto active_ap_var =
        dbus.get_property("org.freedesktop.NetworkManager", device_path,
                          "org.freedesktop.NetworkManager.Device.Wireless", "ActiveAccessPoint");

    if (std::holds_alternative<std::string>(active_ap_var))
    {
        std::string ap_path = std::get<std::string>(active_ap_var);
        if (ap_path != "/" && !ap_path.empty())
        {
            auto strength_var =
                dbus.get_property("org.freedesktop.NetworkManager", ap_path,
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
    // Restoring the correct symbolic names as requested in Turn 3
    if (strength > 85)
        return "nm-signal-100-symbolic";
    if (strength > 65)
        return "nm-signal-75-symbolic";
    if (strength > 40)
        return "nm-signal-50-symbolic";
    if (strength > 15)
        return "nm-signal-25-symbolic";
    return "nm-signal-0-symbolic";
}

int NetworkIndicator::preferred_width() const
{
    return 24;
}
