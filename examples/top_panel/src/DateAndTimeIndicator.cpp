#include "DateAndTimeIndicator.hpp"
#include <ctime>
#include <horizon/WaylandWindow.hpp>
#include <iomanip>
#include <sstream>
#include <horizon/Notification.hpp>

using namespace horizon;

DateAndTimeIndicator::DateAndTimeIndicator() : ITopPanelWidget()
{
    auto label = std::make_unique<Label>("--:--");
    m_label = label.get();
    m_label->set_vertical_alignment(VerticalAlignment::Middle);
    add_child(std::move(label));

    // Wait until we have an application to start the timer.
    when_application_load.connect(
        [this](EventContext &)
        {
            update_time();

            // Update every 60 seconds (but usually we align with the 00s of the clock).
            // For simplicity, we just update every 10s or 60s.
            if (application())
            {
                m_timer_id = application()->add_timer(30000, [this]() { update_time(); });
            }
        });
}

void DateAndTimeIndicator::update_time()
{
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << now->tm_hour << ":" << std::setfill('0')
       << std::setw(2) << now->tm_min;

    m_label->set_text(ss.str());

    // Update tooltip with full date
    char date_buf[128];
    // Using Spanish localization format for the user
    std::strftime(date_buf, sizeof(date_buf), "%A, %d de %B de %Y", now);
    
    auto tip = std::make_unique<Notification>();
    tip->set_notification("office-calendar", date_buf);
    set_tooltip(std::move(tip));

    // Invalidate to force recalculation of layout since the text width might change slightly
    // though HH:mm is mostly constant with monospaced fonts or similar.
    invalidate();
}

int DateAndTimeIndicator::preferred_width() const
{
    return m_label->preferred_width();
}
