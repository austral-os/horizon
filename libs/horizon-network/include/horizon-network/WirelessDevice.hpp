#pragma once

#include <horizon-network/NetworkTypes.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/EventsManager.hpp>
#include <memory>
#include <vector>
#include <string>

namespace horizon::network
{
    class WirelessDevice
    {
    public:
        WirelessDevice(const std::string& name, const std::string& path);

        std::string name() const { return m_name; }
        std::string path() const { return m_path; }

        void request_scan();
        std::vector<WifiNetwork> get_access_points();
        
        bool connect(const std::string& ssid, const std::string& password, const std::string& ap_path);
        bool disconnect(const std::string& ssid);
        
        std::string get_active_ssid();

        // Signals
        EventsManager<EventContext> when_scan_finished;

    private:
        std::string m_name;
        std::string m_path;
        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        
        std::string get_security_string(uint32_t wpa, uint32_t rsn);
    };
}
