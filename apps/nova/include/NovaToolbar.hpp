#pragma once

#include <horizon/EventsManager.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    class GroupButton;
    class SearchBox;
} // namespace horizon

namespace horizon::nova
{

    struct NavigationButtonClickEvent : public EventContext
    {
        int index; // 0 for back, 1 for forward
    };

    struct HomeButtonClickEvent : public EventContext
    {
    };

    struct BookmarkButtonClickEvent : public EventContext
    {
    };

    struct OptionsButtonClickEvent : public EventContext
    {
    };

    struct SearchChangedEvent : public EventContext
    {
        std::string query;
    };

    class NovaToolbar : public Widget
    {
    public:
        NovaToolbar();
        ~NovaToolbar() override = default;

        EventsManager<NavigationButtonClickEvent> when_navigation_clicked;
        EventsManager<HomeButtonClickEvent> when_home_clicked;
        EventsManager<SearchChangedEvent> when_search_submitted;
        EventsManager<BookmarkButtonClickEvent> when_bookmark_clicked;
        EventsManager<OptionsButtonClickEvent> when_options_clicked;

        void set_url(const std::string& url);
        std::string get_url() const;

        void show_add_tab_button(bool show);
        horizon::GroupButton* add_tab_button() const { return m_add_tab_group; }

    private:
        horizon::GroupButton *m_navigation;
        horizon::GroupButton *m_home_group;
        horizon::SearchBox *m_search_box;
        horizon::GroupButton *m_actions_group;
        horizon::GroupButton *m_add_tab_group;
    };

} // namespace horizon::nova
