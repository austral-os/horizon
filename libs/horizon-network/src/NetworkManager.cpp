#include <horizon-network/NetworkManager.hpp>
#include <horizon-network/WirelessDevice.hpp>
#include <dbus/dbus.h>
#include <iostream>
#include <arpa/inet.h>

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

    static std::string prefix_to_mask(uint32_t prefix)
    {
        if (prefix > 32) prefix = 32;
        uint32_t mask = (prefix == 0) ? 0 : (0xFFFFFFFF << (32 - prefix));
        struct in_addr addr;
        addr.s_addr = htonl(mask);
        return inet_ntoa(addr);
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
            
            // NM_DEVICE_STATE_ACTIVATED is 100. But sometimes it's reported as something else if it's still connecting.
            // Let's also check if it's currently connected via Connectivity property or just use state >= 100
            dev.connected = (state == 100);
            dev.status_text = (dev.connected ? "Connected" : "Disconnected");

            if (dev.connected)
            {
                auto ip4_path_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                                      "org.freedesktop.NetworkManager.Device", "Ip4Config");
                if (std::holds_alternative<std::string>(ip4_path_var)) {
                    std::string ip4_path = std::get<std::string>(ip4_path_var);
                    if (ip4_path != "/" && !ip4_path.empty()) {
                        DBusMessage* prop_msg = dbus_message_new_method_call(
                            "org.freedesktop.NetworkManager", ip4_path.c_str(),
                            "org.freedesktop.DBus.Properties", "Get");
                        const char* iface = "org.freedesktop.NetworkManager.IP4Config";
                        const char* prop = "AddressData";
                        dbus_message_append_args(prop_msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);
                        
                        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_dbus->get_connection(), prop_msg, -1, nullptr);
                        if (reply) {
                            DBusMessageIter iter, var_iter, outer_array_iter, dict_iter, entry_iter;
                            dbus_message_iter_init(reply, &iter);
                            
                            // Property Get returns a Variant
                            if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT) {
                                dbus_message_iter_recurse(&iter, &var_iter);
                                // AddressData is aa{sv} (Array of Dictionaries)
                                if (dbus_message_iter_get_arg_type(&var_iter) == DBUS_TYPE_ARRAY) {
                                    dbus_message_iter_recurse(&var_iter, &outer_array_iter);
                                    
                                    // Get the first dictionary in the array
                                    if (dbus_message_iter_get_arg_type(&outer_array_iter) == DBUS_TYPE_ARRAY) {
                                        dbus_message_iter_recurse(&outer_array_iter, &dict_iter);
                                        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                                            dbus_message_iter_recurse(&dict_iter, &entry_iter);
                                            const char* key;
                                            dbus_message_iter_get_basic(&entry_iter, &key);
                                            dbus_message_iter_next(&entry_iter);
                                            
                                            DBusMessageIter val_var_iter;
                                            dbus_message_iter_recurse(&entry_iter, &val_var_iter);
                                            
                                            if (std::string(key) == "address") {
                                                const char* addr;
                                                dbus_message_iter_get_basic(&val_var_iter, &addr);
                                                dev.ip_address = addr;
                                            } else if (std::string(key) == "prefix") {
                                                uint32_t prefix;
                                                dbus_message_iter_get_basic(&val_var_iter, &prefix);
                                                dev.subnet_mask = prefix_to_mask(prefix);
                                            }
                                            dbus_message_iter_next(&dict_iter);
                                        }
                                    }
                                    // Sometimes NM returns a{sv} directly if it's simplified or I misread the signature?
                                    // Actually AddressData is aa{sv}. Let's try to handle a{sv} inside the array.
                                    else if (dbus_message_iter_get_arg_type(&outer_array_iter) == DBUS_TYPE_DICT_ENTRY) {
                                        // This means it's an array of dict entries directly? (a{sv})
                                        // Let's reuse the logic
                                        DBusMessageIter& dict_iter2 = outer_array_iter;
                                        while (dbus_message_iter_get_arg_type(&dict_iter2) == DBUS_TYPE_DICT_ENTRY) {
                                            dbus_message_iter_recurse(&dict_iter2, &entry_iter);
                                            const char* key;
                                            dbus_message_iter_get_basic(&entry_iter, &key);
                                            dbus_message_iter_next(&entry_iter);
                                            DBusMessageIter val_var_iter;
                                            dbus_message_iter_recurse(&entry_iter, &val_var_iter);
                                            if (std::string(key) == "address") {
                                                const char* addr;
                                                dbus_message_iter_get_basic(&val_var_iter, &addr);
                                                dev.ip_address = addr;
                                            } else if (std::string(key) == "prefix") {
                                                uint32_t prefix;
                                                dbus_message_iter_get_basic(&val_var_iter, &prefix);
                                                dev.subnet_mask = prefix_to_mask(prefix);
                                            }
                                            dbus_message_iter_next(&dict_iter2);
                                        }
                                    }
                                }
                            }
                            dbus_message_unref(reply);
                        }
                        dbus_message_unref(prop_msg);

                        auto gateway_var = m_dbus->get_property("org.freedesktop.NetworkManager", ip4_path,
                                                             "org.freedesktop.NetworkManager.IP4Config", "Gateway");
                        if (std::holds_alternative<std::string>(gateway_var)) dev.router = std::get<std::string>(gateway_var);
                    }
                }
            }
            
            if (dev.ip_address.empty()) dev.ip_address = "---";
            if (dev.subnet_mask.empty()) dev.subnet_mask = "---";
            if (dev.router.empty()) dev.router = "---";
            if (dev.dns.empty()) dev.dns = "---";
            dev.config_method = "Using DHCP";

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
