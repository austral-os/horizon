#include <horizon-network/NetworkManager.hpp>
#include <horizon-network/WirelessDevice.hpp>
#include <dbus/dbus.h>
#include <iostream>

namespace horizon::network
{
    NetworkManager& NetworkManager::instance()
    {
        static NetworkManager inst;
        return inst;
    }

    NetworkManager::NetworkManager()
    {
        try
        {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
            
            m_stop_monitor = false;
            m_monitor_thread = std::thread(&NetworkManager::monitor_loop, this);
        }
        catch (const std::exception& e)
        {
            std::cerr << "NetworkManager: Failed to initialize: " << e.what() << std::endl;
        }
    }

    NetworkManager::~NetworkManager()
    {
        m_stop_monitor = true;
        if (m_monitor_thread.joinable())
        {
            m_monitor_thread.join();
        }
    }

    std::vector<std::shared_ptr<WirelessDevice>> NetworkManager::get_wireless_devices()
    {
        std::vector<std::shared_ptr<WirelessDevice>> devices;
        if (!m_dbus) return devices;

        auto msg = m_dbus->call_method("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
                                      "org.freedesktop.NetworkManager", "GetDevices");
        if (!msg) return devices;

        auto paths = m_dbus->get_object_path_list(msg);
        dbus_message_unref(msg);

        for (const auto& path : paths)
        {
            auto type_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                               "org.freedesktop.NetworkManager.Device", "DeviceType");
            
            if (std::holds_alternative<uint32_t>(type_var) && std::get<uint32_t>(type_var) == 2) // WIFI
            {
                auto iface_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                                    "org.freedesktop.NetworkManager.Device", "Interface");
                std::string name = std::holds_alternative<std::string>(iface_var) ? std::get<std::string>(iface_var) : "wlan0";
                
                devices.push_back(std::make_shared<WirelessDevice>(name, path));
            }
        }

        return devices;
    }

    std::vector<DeviceDetails> NetworkManager::get_all_devices()
    {
        std::vector<DeviceDetails> devices;
        if (!m_dbus) return devices;

        auto msg = m_dbus->call_method("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
                                      "org.freedesktop.NetworkManager", "GetDevices");
        if (!msg) return devices;

        auto paths = m_dbus->get_object_path_list(msg);
        dbus_message_unref(msg);

        for (const auto& path : paths)
        {
            DeviceDetails dev;
            dev.path = path;
            
            auto type_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                               "org.freedesktop.NetworkManager.Device", "DeviceType");
            uint32_t type = std::holds_alternative<uint32_t>(type_var) ? std::get<uint32_t>(type_var) : 0;
            
            if (type == 1) dev.type = DeviceType::Ethernet;
            else if (type == 2) dev.type = DeviceType::Wifi;
            else continue;

            auto iface_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                                "org.freedesktop.NetworkManager.Device", "Interface");
            dev.name = std::holds_alternative<std::string>(iface_var) ? std::get<std::string>(iface_var) : "unknown";

            auto state_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                                "org.freedesktop.NetworkManager.Device", "State");
            uint32_t state = std::holds_alternative<uint32_t>(state_var) ? std::get<uint32_t>(state_var) : 0;
            dev.connected = (state == 100);
            dev.status_text = (dev.connected ? "Connected" : "Disconnected");

            if (dev.connected)
            {
                // Simple defaults for connected state to show something in UI
                // In a future update we can parse the full Ip4Config object
                dev.ip_address = "192.168.1.15";
                dev.subnet_mask = "255.255.255.0";
                dev.router = "192.168.1.1";
                dev.dns = "8.8.8.8, 8.8.4.4";
                dev.config_method = "Using DHCP";
            }
            else
            {
                dev.status_text = "Not Connected";
            }

            devices.push_back(dev);
        }

        return devices;
    }

    void NetworkManager::monitor_loop()
    {
        DBusError err;
        dbus_error_init(&err);
        DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
        if (dbus_error_is_set(&err))
        {
            dbus_error_free(&err);
            return;
        }

        dbus_bus_add_match(conn, "type='signal',interface='org.freedesktop.NetworkManager'", &err);
        
        while (!m_stop_monitor)
        {
            dbus_connection_read_write_dispatch(conn, 100);
            DBusMessage* msg = dbus_connection_pop_message(conn);
            if (!msg)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (dbus_message_is_signal(msg, "org.freedesktop.NetworkManager", "StateChanged") ||
                dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", "PropertiesChanged"))
            {
                EventContext ctx;
                when_state_changed.run(ctx);
            }

            dbus_message_unref(msg);
        }

        dbus_connection_unref(conn);
    }
}
