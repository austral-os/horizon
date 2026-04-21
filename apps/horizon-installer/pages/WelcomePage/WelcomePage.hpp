#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon::installer
{
    /**
     * @brief Page 1: Welcome / Presentation
     */
    class WelcomePage : public Widget
    {
    public:
        WelcomePage();
        ~WelcomePage() override = default;

        EventsManager<EventContext> when_continue;
    };
} // namespace horizon::installer
