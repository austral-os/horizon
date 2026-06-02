#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Icon.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include "ITopPanelWidget.hpp"

namespace horizon
{

    class UpdateIndicator : public ITopPanelWidget
    {
    public:
        UpdateIndicator();
        ~UpdateIndicator() override;

        std::string widget_name() const override { return "UpdateIndicator"; }
        std::string widget_id() const override { return "update_indicator"; }

        int preferred_width() const override;

    private:
        std::unique_ptr<Icon> m_icon;
        
        std::thread m_monitor_thread;
        std::atomic<bool> m_running{false};
        
        int check_updates();
        void update_ui(int update_count);
    };

} // namespace horizon
