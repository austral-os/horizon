#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace horizon::network
{
    enum class DeviceType
    {
        Unknown = 0,
        Ethernet = 1,
        Wifi = 2
    };

    struct DeviceDetails
    {
        std::string name;
        std::string connection_name;
        std::string path;

        DeviceType type;
        bool connected;
        std::string status_text;
        std::string ip_address;
        std::string subnet_mask;
        std::string router;
        std::string dns;
        std::string search_domains;
        std::string config_method; // e.g. "DHCP"
    };

    struct WifiNetwork
    {
        std::string ssid;
        std::string security;
        std::string path; // D-Bus object path for the AccessPoint
        int signal;
        bool connected;
    };

    enum class ConnectionState
    {
        Unknown = 0,
        Activating = 1,
        Activated = 2,
        Deactivating = 3,
        Deactivated = 4,
        Failed = 5
    };
}
