#pragma once

#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    class GroupButton;
} // namespace horizon

namespace horizon::terminal
{

    struct NewTabClickEvent : public EventContext
    {
    };

    struct FullscreenClickEvent : public EventContext
    {
    };

    struct PreferencesClickEvent : public EventContext
    {
    };

    class TerminalToolbar : public Widget
    {
    public:
        TerminalToolbar();
        ~TerminalToolbar() override = default;

        EventsManager<NewTabClickEvent> when_new_tab_clicked;
        EventsManager<FullscreenClickEvent> when_fullscreen_clicked;
        EventsManager<PreferencesClickEvent> when_preferences_clicked;

    private:
        horizon::GroupButton *m_new_tab_group;
        horizon::GroupButton *m_settings_group;
    };

} // namespace horizon::terminal
