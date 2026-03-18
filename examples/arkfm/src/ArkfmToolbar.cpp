#include "ArkfmToolbar.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/ToggleGroupButton.hpp>

namespace horizon::arkfm
{

    ArkToolbar::ArkToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(5);

        auto navigation = std::make_unique<horizon::GroupButton>();
        m_navigation = navigation.get();
        m_navigation->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_navigation->set_fixed_size(80);

        auto mini_spacer1 = std::make_unique<Widget>();
        mini_spacer1->set_fixed_size(20);

        auto back_icon = std::make_unique<horizon::Icon>();
        back_icon->set_icon_name("go-previous");
        back_icon->set_icon_size(16);
        m_navigation->add_item(std::move(back_icon));

        auto forward_icon = std::make_unique<horizon::Icon>();
        forward_icon->set_icon_name("go-next");
        forward_icon->set_icon_size(16);
        m_navigation->add_item(std::move(forward_icon));

        auto view_modes = std::make_unique<horizon::ToggleGroupButton>();
        m_view_modes = view_modes.get();
        m_view_modes->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_view_modes->set_fixed_size(140);
        m_view_modes->set_margin(0);

        auto icon_view = std::make_unique<horizon::Icon>();
        icon_view->set_icon_name("view-grid");
        icon_view->set_icon_size(16);
        m_view_modes->add_item(std::move(icon_view));

        auto list_view = std::make_unique<horizon::Icon>();
        list_view->set_icon_name("view-filter");
        list_view->set_icon_size(16);
        m_view_modes->add_item(std::move(list_view));

        auto column_view = std::make_unique<horizon::Icon>();
        column_view->set_icon_name("view-column");
        column_view->set_icon_size(16);
        m_view_modes->add_item(std::move(column_view));

        auto cover_flow = std::make_unique<horizon::Icon>();
        cover_flow->set_icon_name("view-coverflow"); // Generic enough for cover flow demo
        cover_flow->set_icon_size(16);
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
        add_child(std::move(mini_spacer1));
        add_child(std::move(view_modes));
        add_child(std::move(spacer));
        add_child(std::move(search_box));
    }

} // namespace horizon::arkfm