#pragma once

#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon::arkfm
{

    struct NavigationButtonClickEvent : public EventContext
    {
        int index; // 0 for back, 1 for forward
    };

    class ArkToolbar : public Widget
    {
    public:
        ArkToolbar();
        ~ArkToolbar() override = default;

        EventsManager<NavigationButtonClickEvent> when_navigation_clicked;
    };

} // namespace horizon::arkfm