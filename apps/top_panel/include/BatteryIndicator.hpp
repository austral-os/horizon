#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

/**
 * @brief Indicator that shows battery status and percentage in the top panel.
 */
class BatteryIndicator : public ITopPanelWidget
{
public:
    BatteryIndicator();
    virtual ~BatteryIndicator();

    std::string widget_id() const override { return "battery_status"; }
    std::string widget_name() const override { return "Battery Status"; }

    int preferred_width() const override;

private:
    void monitor_loop();
    void update_ui(const std::string& icon_name, double percentage, bool is_present);
    
    struct BatteryStatus {
        std::string icon_name;
        double percentage{0.0};
        bool is_present{false};
    };
    BatteryStatus calculate_current_info();

    horizon::Icon* m_icon;
    horizon::Label* m_label;
    
    std::thread m_monitor_thread;
    std::atomic<bool> m_stop_monitor{false};
    
    std::mutex m_state_mutex;
    std::string m_current_icon_name;
    double m_current_percentage{-1.0};
    bool m_current_is_present{false};
};
