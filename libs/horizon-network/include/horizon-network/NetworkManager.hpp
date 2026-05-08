#pragma once

#include <horizon-network/NetworkTypes.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <horizon/EventsManager.hpp>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

namespace horizon::network
{
    class WirelessDevice;

    class NetworkManager
    {
    public:
        static NetworkManager& instance();

        NetworkManager();
        ~NetworkManager();

        std::vector<std::shared_ptr<WirelessDevice>> get_wireless_devices();
        
        // Signals
        EventsManager<EventContext> when_state_changed;
        EventsManager<EventContext> when_device_added;

    private:
        void monitor_loop();

        std::unique_ptr<dbusutils::DbusHelper> m_dbus;
        std::thread m_monitor_thread;
        std::atomic<bool> m_stop_monitor{false};
    };
}
