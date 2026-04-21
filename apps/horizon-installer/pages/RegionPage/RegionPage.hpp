#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <string>

namespace horizon::installer
{
    /**
     * @brief OOBE Page: Region & Language configuration.
     */
    class RegionPage : public Widget
    {
    public:
        RegionPage();
        ~RegionPage() override = default;

        std::string selected_country() const { return m_selected_country; }
        std::string selected_timezone() const { return m_selected_timezone; }

        EventsManager<EventContext> when_continue;
        EventsManager<EventContext> when_back;

    private:
        std::string m_selected_country;
        std::string m_selected_timezone;
    };
} // namespace horizon::installer
