#include "horizon/files/FileToolbar.hpp"
#include "horizon/Spacer.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Notification.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/ToggleGroupButton.hpp>

namespace horizon::files
{
    FileToolbar::FileToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(5);

        auto navigation = std::make_unique<horizon::GroupButton>();
        m_navigation = navigation.get();
        m_navigation->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_navigation->set_fixed_size(80);

        auto path_button = std::make_unique<horizon::GroupButton>();
        path_button->set_fixed_size(200);
        m_path_button = path_button.get();
        m_path_button->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        m_path_button->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &ctx)
            {
                if (ctx.button_index == 0)
                {
                    horizon::EventContext ev;
                    this->when_go_up_clicked.run(ev);
                }
            });

        auto mini_spacer1 = std::make_unique<Widget>();
        mini_spacer1->set_fixed_size(20);

        auto back_icon = std::make_unique<horizon::Icon>();
        back_icon->set_icon_name("go-previous");
        back_icon->set_icon_size(16);
        back_icon->set_use_theme_colors(true);
        m_navigation->add_item(std::move(back_icon));

        auto forward_icon = std::make_unique<horizon::Icon>();
        forward_icon->set_icon_name("go-next");
        forward_icon->set_icon_size(16);
        forward_icon->set_use_theme_colors(true);
        m_navigation->add_item(std::move(forward_icon));

        auto view_modes = std::make_unique<horizon::ToggleGroupButton>();
        m_view_modes = view_modes.get();
        m_view_modes->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_view_modes->set_fixed_size(140);
        m_view_modes->set_margin(0);

        auto icon_view = std::make_unique<horizon::Icon>();
        icon_view->set_icon_name("view-grid");
        icon_view->set_icon_size(16);
        icon_view->set_use_theme_colors(true);
        m_view_modes->add_item(std::move(icon_view));

        auto list_view = std::make_unique<horizon::Icon>();
        list_view->set_icon_name("view-filter");
        list_view->set_icon_size(16);
        list_view->set_use_theme_colors(true);
        m_view_modes->add_item(std::move(list_view));

        auto column_view = std::make_unique<horizon::Icon>();
        column_view->set_icon_name("view-column");
        column_view->set_icon_size(16);
        column_view->set_use_theme_colors(true);
        m_view_modes->add_item(std::move(column_view));

        auto cover_flow = std::make_unique<horizon::Icon>();
        cover_flow->set_icon_name("view-coverflow");
        cover_flow->set_icon_size(16);
        cover_flow->set_use_theme_colors(true);
        m_view_modes->add_item(std::move(cover_flow));

        m_navigation->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &ctx)
            {
                NavigationButtonClickEvent nav_event;
                nav_event.index = ctx.button_index;
                this->when_navigation_clicked.run(nav_event);
            });

        m_view_modes->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &ctx)
            {
                ViewModeChangeEvent view_event;
                view_event.view_mode_index = ctx.button_index;
                this->when_view_mode_changed.run(view_event);
            });

        auto search_box = std::make_unique<horizon::SearchBox>();
        m_search_box = search_box.get();
        m_search_box->set_placeholder("Buscar...");
        m_search_box->set_fixed_size(200);

        m_search_box->when_text_changed.connect(
            [this](KeyEventContext &)
            {
                SearchChangedEvent ev;
                ev.query = m_search_box->text();
                this->when_search_changed.run(ev);
            });

        auto spacer = std::make_unique<Widget>();
        spacer->set_position_type(FILL);

        add_child(std::move(navigation));

        auto mini_spacer_path = std::make_unique<Widget>();
        mini_spacer_path->set_fixed_size(10);
        add_child(std::move(mini_spacer_path));

        add_child(std::move(view_modes));
        add_child(Spacer());
        add_child(std::move(path_button));
        add_child(Spacer());
        add_child(std::move(search_box));
    }

    void FileToolbar::update_navigation_state(bool can_back, bool can_forward)
    {
        // For now, GroupButton doesn't support individual item enabling easily via public API,
        // but it's part of the plan to improve it.
    }

    void FileToolbar::update_path(const std::string &path)
    {
        std::filesystem::path p(path);
        std::string folder_name = p.filename().string();
        if (path == "/" || folder_name.empty())
        {
            folder_name = "/";
        }

        m_path_button->clear_children();

        auto up_icon = std::make_unique<horizon::Icon>();
        up_icon->set_icon_name("go-up");
        up_icon->set_icon_size(16);
        up_icon->set_fixed_size(32);
        up_icon->set_use_theme_colors(true);
        m_path_button->add_item(std::move(up_icon), 40);

        m_path_button->add_item(folder_name);

        if (m_path_button->children().size() >= 2)
        {
            auto tooltip = std::make_unique<horizon::Notification>();
            tooltip->set_message(path);
            m_path_button->children()[1]->set_tooltip(std::move(tooltip));
        }
    }
} // namespace horizon::files
