#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon::installer
{
    /**
     * @brief Page 2: License Agreement
     */
    class LicensePage : public Widget
    {
    public:
        LicensePage();
        ~LicensePage() override = default;

        EventsManager<EventContext> when_agree;
        EventsManager<EventContext> when_disagree;
    };
} // namespace horizon::installer
