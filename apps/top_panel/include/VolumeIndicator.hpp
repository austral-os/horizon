#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Icon.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Vault.hpp>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace horizon
{

class VolumeIndicator : public ITopPanelWidget
{
public:
    VolumeIndicator();
    ~VolumeIndicator() override;

    std::string widget_id() const override { return "volume"; }
    std::string widget_name() const override { return "Volume"; }
    
    int preferred_width() const override;

private:
    void monitor_loop();
    double get_current_volume();
    bool get_current_mute();
    void update_ui(double volume, bool is_muted);

    Icon *m_icon{nullptr};
    Slider *m_slider{nullptr};

    std::thread m_monitor_thread;
    std::atomic<bool> m_stop_monitor{false};
    std::mutex m_state_mutex;

    double m_current_volume{-1.0};
    bool m_current_muted{false};
};

} // namespace horizon
