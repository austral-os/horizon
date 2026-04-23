#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon::installer
{
    /**
     * @brief Final success page.
     */
    class SuccessPage : public Widget
    {
    public:
        SuccessPage(bool is_oobe = false);
        ~SuccessPage() override = default;

        EventsManager<EventContext> when_finish;
    };
} // namespace horizon::installer
