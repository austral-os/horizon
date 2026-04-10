#include "NovaToolbar.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Logger.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/I18n.hpp>

namespace horizon::nova
{

    NovaToolbar::NovaToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(5);
        set_spacing(10);

        // 1. Navigation Buttons (Back/Forward)
        auto navigation = std::make_unique<horizon::GroupButton>();
        m_navigation = navigation.get();
        m_navigation->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_navigation->set_fixed_size(80);

        auto back_icon = std::make_unique<horizon::Icon>();
        back_icon->set_icon_name("go-previous");
        back_icon->set_icon_size(16);
        m_navigation->add_item(std::move(back_icon));

        auto forward_icon = std::make_unique<horizon::Icon>();
        forward_icon->set_icon_name("go-next");
        forward_icon->set_icon_size(16);
        m_navigation->add_item(std::move(forward_icon));

        m_navigation->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &ctx)
            {
                NavigationButtonClickEvent nav_event;
                nav_event.index = ctx.button_index;
                this->when_navigation_clicked.run(nav_event);
            });

        // 2. Home Button
        auto home_group = std::make_unique<horizon::GroupButton>();
        m_home_group = home_group.get();
        m_home_group->set_fixed_size(40);

        auto home_icon = std::make_unique<horizon::Icon>();
        home_icon->set_icon_name("go-home");
        home_icon->set_icon_size(16);
        m_home_group->add_item(std::move(home_icon));

        m_home_group->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &)
            {
                HomeButtonClickEvent ev;
                this->when_home_clicked.run(ev);
            });

        // 3. Search Box (URL) - This one FILLs the space
        auto search_box = std::make_unique<horizon::SearchBox>();
        m_search_box = search_box.get();
        m_search_box->set_placeholder(i18n().tr("nova.placeholder"));
        m_search_box->set_position_type(FILL);
        m_search_box->set_fixed_size(-1); // Override default TextBox fixed size to allow expansion

        m_search_box->when_key_press.connect(
            [this](KeyEventContext &ctx)
            {
                if (ctx.keysym == 0xff0d || ctx.keysym == 0xff8d)
                { // Enter key or KP_Enter
                    std::string query = m_search_box->text();
                    LOG_INFO << "[NOVA] Search submitted: " << query;
                    SearchChangedEvent ev;
                    ev.query = query;
                    this->when_search_submitted.run(ev);
                }
            });

        m_search_box->when_click.connect(
            [this](EventContext &)
            {
                if (application())
                {
                    application()->post_task([this]() { m_search_box->select_all(); });
                }
            });

        // 4. Actions Group (Bookmarks/Options)
        auto actions_group = std::make_unique<horizon::GroupButton>();
        m_actions_group = actions_group.get();
        m_actions_group->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_actions_group->set_fixed_size(80);

        auto bookmark_icon = std::make_unique<horizon::Icon>();
        bookmark_icon->set_icon_name("bookmarks");
        bookmark_icon->set_icon_size(16);
        m_actions_group->add_item(std::move(bookmark_icon));

        auto options_icon = std::make_unique<horizon::Icon>();
        options_icon->set_icon_name("emblem-system");
        options_icon->set_icon_size(16);
        m_actions_group->add_item(std::move(options_icon));

        m_actions_group->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &ctx)
            {
                if (ctx.button_index == 0)
                {
                    BookmarkButtonClickEvent ev;
                    this->when_bookmark_clicked.run(ev);
                }
                else
                {
                    OptionsButtonClickEvent ev;
                    this->when_options_clicked.run(ev);
                }
            });

        add_child(std::move(navigation));
        add_child(std::move(home_group));
        add_child(std::move(search_box));
        add_child(std::move(actions_group));
    }

    void NovaToolbar::set_url(const std::string &url)
    {
        if (m_search_box)
            m_search_box->set_text(url);
    }

    std::string NovaToolbar::get_url() const
    {
        return m_search_box ? m_search_box->text() : "";
    }

} // namespace horizon::nova
