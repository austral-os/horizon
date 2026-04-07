#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Icon.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <string>
#include <thread>
#include <mutex>
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
    void update_ui(const std::string& icon_name, const std::string& status_text);
    
    // Asynchronous logic performed in background thread
    struct NetworkStatus {
        std::string icon_name;
        std::string status_text;
    };
    NetworkStatus calculate_current_info();
    int get_wifi_signal_strength(horizon::dbusutils::DbusHelper& dbus, const std::string& device_path);
    std::string get_icon_for_wifi(int strength);

    horizon::Icon* m_icon;
    
    std::thread m_monitor_thread;
    std::atomic<bool> m_stop_monitor{false};
    
    // Thread-safe state
    std::mutex m_state_mutex;
    std::string m_current_icon_name;
    std::string m_current_status_text;
};
