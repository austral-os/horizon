#include <horizon-network/NetworkManager.hpp>
#include <horizon-network/WirelessDevice.hpp>
#include <dbus/dbus.h>
#include <iostream>
#include <arpa/inet.h>
#include <sstream>
#include <horizon/Logger.hpp>


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

    static uint32_t mask_to_prefix(const std::string& mask)
    {
        uint32_t m;
        inet_pton(AF_INET, mask.c_str(), &m);
        m = ntohl(m);
        uint32_t prefix = 0;
        while (m & 0x80000000) {
            prefix++;
            m <<= 1;
        }
        return prefix;
    }

    static void parse_aasv(DBusMessageIter* iter, std::function<void(const std::string&, DBusMessageIter*)> callback)
    {
        DBusMessageIter variant_iter, array_iter, sub_array_iter, dict_iter, entry_iter;
        
        if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_VARIANT) return;
        dbus_message_iter_recurse(iter, &variant_iter);
        
        if (dbus_message_iter_get_arg_type(&variant_iter) != DBUS_TYPE_ARRAY) return;
        dbus_message_iter_recurse(&variant_iter, &array_iter);
        
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_ARRAY)
        {
            dbus_message_iter_recurse(&array_iter, &sub_array_iter);
            while (dbus_message_iter_get_arg_type(&sub_array_iter) == DBUS_TYPE_DICT_ENTRY)
            {
                dbus_message_iter_recurse(&sub_array_iter, &dict_iter);
                const char* key;
                dbus_message_iter_get_basic(&dict_iter, &key);
                dbus_message_iter_next(&dict_iter);
                
                DBusMessageIter val_var_iter;
                dbus_message_iter_recurse(&dict_iter, &val_var_iter);
                
                callback(std::string(key), &val_var_iter);
                
                dbus_message_iter_next(&sub_array_iter);
            }
            dbus_message_iter_next(&array_iter);
        }
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
            dev.ip_address = "---";
            dev.subnet_mask = "---";
            dev.router = "---";
            dev.dns = "---";
            dev.connection_name = "---";
            dev.config_method = "Using DHCP";
            
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
                auto active_conn_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                                         "org.freedesktop.NetworkManager.Device", "ActiveConnection");
                if (std::holds_alternative<std::string>(active_conn_var)) {
                    std::string active_path = std::get<std::string>(active_conn_var);
                    if (active_path != "/" && !active_path.empty()) {
                        auto id_var = m_dbus->get_property("org.freedesktop.NetworkManager", active_path,
                                                         "org.freedesktop.NetworkManager.Connection.Active", "Id");
                        if (std::holds_alternative<std::string>(id_var)) dev.connection_name = std::get<std::string>(id_var);
                    }
                }

                auto ip4_path_var = m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                                      "org.freedesktop.NetworkManager.Device", "Ip4Config");
                if (std::holds_alternative<std::string>(ip4_path_var)) {
                    std::string ip4_path = std::get<std::string>(ip4_path_var);
                    if (ip4_path != "/" && !ip4_path.empty()) {
                        DBusMessage* prop_msg = dbus_message_new_method_call("org.freedesktop.NetworkManager", ip4_path.c_str(), "org.freedesktop.DBus.Properties", "Get");
                        const char* iface = "org.freedesktop.NetworkManager.IP4Config";
                        const char* prop = "AddressData";
                        dbus_message_append_args(prop_msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);
                        DBusMessage* reply = dbus_connection_send_with_reply_and_block(m_dbus->get_connection(), prop_msg, -1, nullptr);
                        if (reply) {
                            DBusMessageIter iter;
                            dbus_message_iter_init(reply, &iter);
                            parse_aasv(&iter, [&](const std::string& key, DBusMessageIter* val) {
                                if (key == "address") {
                                    const char* addr;
                                    dbus_message_iter_get_basic(val, &addr);
                                    dev.ip_address = addr;
                                } else if (key == "prefix") {
                                    uint32_t prefix;
                                    dbus_message_iter_get_basic(val, &prefix);
                                    dev.subnet_mask = prefix_to_mask(prefix);
                                }
                            });
                            dbus_message_unref(reply);
                        }
                        dbus_message_unref(prop_msg);

                        auto gateway_var = m_dbus->get_property("org.freedesktop.NetworkManager", ip4_path, "org.freedesktop.NetworkManager.IP4Config", "Gateway");
                        if (std::holds_alternative<std::string>(gateway_var)) dev.router = std::get<std::string>(gateway_var);

                        DBusMessage* dns_msg = dbus_message_new_method_call("org.freedesktop.NetworkManager", ip4_path.c_str(), "org.freedesktop.DBus.Properties", "Get");
                        const char* dns_prop = "NameserverData";
                        dbus_message_append_args(dns_msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &dns_prop, DBUS_TYPE_INVALID);
                        DBusMessage* dns_reply = dbus_connection_send_with_reply_and_block(m_dbus->get_connection(), dns_msg, -1, nullptr);
                        if (dns_reply) {
                            DBusMessageIter iter;
                            dbus_message_iter_init(dns_reply, &iter);
                            std::stringstream ss;
                            bool first = true;
                            parse_aasv(&iter, [&](const std::string& key, DBusMessageIter* val) {
                                if (key == "address") {
                                    const char* addr;
                                    dbus_message_iter_get_basic(val, &addr);
                                    if (!first) ss << ", ";
                                    ss << addr;
                                    first = false;
                                }
                            });
                            if (!ss.str().empty()) dev.dns = ss.str();
                            dbus_message_unref(dns_reply);
                        }
                        dbus_message_unref(dns_msg);
                    }
                }
            }

            devices.push_back(dev);
        }

        return devices;
    }

    static void copy_dbus_iterator(DBusMessageIter* src, DBusMessageIter* dest)
    {
        int type = dbus_message_iter_get_arg_type(src);
        if (type == DBUS_TYPE_INVALID) return;

        if (dbus_type_is_container(type)) {
            DBusMessageIter src_sub, dest_sub;
            dbus_message_iter_recurse(src, &src_sub);
            
            char* sig = nullptr;
            const char* contained_sig = nullptr;
            
            if (type == DBUS_TYPE_ARRAY || type == DBUS_TYPE_VARIANT) {
                sig = dbus_message_iter_get_signature(&src_sub);
                contained_sig = sig;
            }

            dbus_message_iter_open_container(dest, type, contained_sig, &dest_sub);
            
            while (dbus_message_iter_get_arg_type(&src_sub) != DBUS_TYPE_INVALID) {
                copy_dbus_iterator(&src_sub, &dest_sub);
                dbus_message_iter_next(&src_sub);
            }
            dbus_message_iter_close_container(dest, &dest_sub);
            if (sig) dbus_free(sig);
        } else {
            // Basic type
            switch (type) {
                case DBUS_TYPE_STRING:
                case DBUS_TYPE_OBJECT_PATH:
                case DBUS_TYPE_SIGNATURE: {
                    const char* val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_BOOLEAN: {
                    dbus_bool_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_BYTE: {
                    unsigned char val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_INT16: {
                    dbus_int16_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_UINT16: {
                    dbus_uint16_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_INT32: {
                    dbus_int32_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_UINT32: {
                    dbus_uint32_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_INT64: {
                    dbus_int64_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_UINT64: {
                    dbus_uint64_t val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_DOUBLE: {
                    double val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
                case DBUS_TYPE_UNIX_FD: {
                    int val;
                    dbus_message_iter_get_basic(src, &val);
                    dbus_message_iter_append_basic(dest, type, &val);
                    break;
                }
            }
        }
    }



    bool NetworkManager::apply_device_settings(const DeviceDetails& details)
    {
        if (!m_dbus) return false;
        LOG_INFO << "[NETWORK] Applying settings to " << details.name << " via nmcli + D-Bus Reapply";

        // Step 1: Get the active connection's path
        auto active_conn_var = m_dbus->get_property("org.freedesktop.NetworkManager", details.path,
                                                    "org.freedesktop.NetworkManager.Device",
                                                    "ActiveConnection");
        if (!std::holds_alternative<std::string>(active_conn_var)) {
            LOG_ERROR << "[NETWORK] No active connection on device " << details.name;
            return false;
        }
        std::string active_conn_path = std::get<std::string>(active_conn_var);

        // Step 2: Get the UUID directly from the device using nmcli (Most robust way)
        std::string target_uuid;
        {
            std::string cmd = "nmcli -g GENERAL.CON-UUID device show " + details.name;
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                char buffer[128];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    target_uuid = buffer;
                    // Remove newline
                    target_uuid.erase(target_uuid.find_last_not_of(" \n\r\t") + 1);
                }
                pclose(pipe);
            }
        }

        if (target_uuid.empty()) {
            LOG_ERROR << "[NETWORK] Could not find active connection UUID for " << details.name;
            return false;
        }

        LOG_INFO << "[NETWORK] Target UUID for " << details.name << ": " << target_uuid;

        // Step 3: Use nmcli to modify the connection
        std::stringstream ss;
        ss << "nmcli connection modify " << target_uuid << " ";


        
        if (details.config_method == "Manual") {
            uint32_t prefix = mask_to_prefix(details.subnet_mask);
            ss << "ipv4.method manual ipv4.addresses " << details.ip_address << "/" << prefix << " ";
            if (!details.router.empty() && details.router != "---") {
                ss << "ipv4.gateway " << details.router << " ";
            }
            if (!details.dns.empty() && details.dns != "---") {
                // NM expects DNS separated by spaces or commas depending on version, nmcli handles it
                ss << "ipv4.dns \"" << details.dns << "\" ";
            }
        } else {
            ss << "ipv4.method auto ipv4.addresses \"\" ipv4.gateway \"\" ipv4.dns \"\" ";
        }

        LOG_INFO << "[NETWORK] Executing: " << ss.str();
        int res = system(ss.str().c_str());
        if (res != 0) {
            LOG_ERROR << "[NETWORK] nmcli failed with code " << res;
            return false;
        }

        // Step 5: Force immediate application via D-Bus Reapply
        DBusMessage* reapply_msg = dbus_message_new_method_call("org.freedesktop.NetworkManager",
                                                               details.path.c_str(),
                                                               "org.freedesktop.NetworkManager.Device",
                                                               "Reapply");
        DBusMessageIter ri, empty_conn;
        dbus_message_iter_init_append(reapply_msg, &ri);
        dbus_message_iter_open_container(&ri, DBUS_TYPE_ARRAY, "{sa{sv}}", &empty_conn);
        dbus_message_iter_close_container(&ri, &empty_conn);

        dbus_uint64_t version_id = 0;
        dbus_message_iter_append_basic(&ri, DBUS_TYPE_UINT64, &version_id);
        dbus_uint32_t reapply_flags = 0;
        dbus_message_iter_append_basic(&ri, DBUS_TYPE_UINT32, &reapply_flags);

        DBusMessage* reapply_reply = dbus_connection_send_with_reply_and_block(
            m_dbus->get_connection(), reapply_msg, 5000, nullptr);
        dbus_message_unref(reapply_msg);

        if (reapply_reply) {
            dbus_message_unref(reapply_reply);
            LOG_INFO << "[NETWORK] Settings applied successfully for " << details.name;
            return true;
        }

        LOG_ERROR << "[NETWORK] Reapply failed after nmcli update";
        return false;
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
