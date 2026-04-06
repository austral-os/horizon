#pragma once
#include <string>

namespace horizon::preferences
{
    struct BluetoothDevice
    {
        std::string name;
        std::string address;
        bool connected;
        bool paired;
        int rssi;
        std::string path;
    };
}
