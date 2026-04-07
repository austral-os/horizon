#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Label.hpp>
#include <string>
#include <memory>

/**
 * @brief Indicator that shows current time in the top panel.
 */
class DateAndTimeIndicator : public ITopPanelWidget
{
public:
    DateAndTimeIndicator();
    virtual ~DateAndTimeIndicator() = default;

    std::string widget_id() const override { return "date_and_time"; }
    std::string widget_name() const override { return "Date and Time"; }

    int preferred_width() const override;

private:
    void update_time();
    
    horizon::Label* m_label;
    size_t m_timer_id = 0;
};
