#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace horizon::network
{
    struct WifiNetwork
    {
        std::string ssid;
        std::string security;
        std::string path; // D-Bus object path for the AccessPoint
        int signal;
        bool connected;
    };

    struct WifiDevice
    {
        std::string name; // interface name (e.g. wlo1)
        std::string path; // D-Bus object path for the Device
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
