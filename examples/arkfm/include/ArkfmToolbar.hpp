#pragma once

#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    class GroupButton;
    class ToggleGroupButton;
} // namespace horizon

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

    private:
        horizon::GroupButton *m_navigation;
        horizon::ToggleGroupButton *m_view_modes;
    };

} // namespace horizon::arkfm