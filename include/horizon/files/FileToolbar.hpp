#pragma once

#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>

namespace horizon
{
    class GroupButton;
    class ToggleGroupButton;
    class SearchBox;
} // namespace horizon

namespace horizon::files
{
    struct NavigationButtonClickEvent : public EventContext
    {
        int index; // 0 for back, 1 for forward
    };

    struct ViewModeChangeEvent : public EventContext
    {
        int view_mode_index;
    };

    struct SearchChangedEvent : public EventContext
    {
        std::string query;
    };

    class FileToolbar : public Widget
    {
    public:
        FileToolbar();
        ~FileToolbar() override = default;

        EventsManager<NavigationButtonClickEvent> when_navigation_clicked;
        EventsManager<ViewModeChangeEvent> when_view_mode_changed;
        EventsManager<SearchChangedEvent> when_search_changed;

        void update_navigation_state(bool can_back, bool can_forward);

    private:
        horizon::GroupButton *m_navigation;
        horizon::ToggleGroupButton *m_view_modes;
        horizon::SearchBox *m_search_box;
    };
} // namespace horizon::files
