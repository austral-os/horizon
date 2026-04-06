#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Icon.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

/**
 * @brief Indicator that shows network connection status (WiFi/Ethernet) in the top panel.
 */
class NetworkIndicator : public ITopPanelWidget
{
public:
    NetworkIndicator();
    virtual ~NetworkIndicator();

    std::string widget_id() const override { return "network_status"; }
    std::string widget_name() const override { return "Network Status"; }

    int preferred_width() const override;

private:
    void setup_dbus();
    void monitor_loop();
    void update_status();
    
    // DBus querying helpers
    std::string get_active_connection_type();
    int get_wifi_signal_strength(const std::string& device_path);
    std::string get_icon_for_wifi(int strength);

    horizon::Icon* m_icon;
    std::unique_ptr<horizon::dbusutils::DbusHelper> m_dbus;
    
    std::thread m_monitor_thread;
    std::atomic<bool> m_stop_monitor{false};
    
    // Cached state to avoid unnecessary repaints
    std::string m_current_icon_name;
};
