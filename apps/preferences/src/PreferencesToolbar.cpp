#include "PreferencesToolbar.hpp"
#include "horizon/Spacer.hpp"
#include "horizon/Widget.hpp"
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    PreferencesToolbar::PreferencesToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_margin(5);
        set_spacing(5);

        // Navigation Group
        auto navigation = std::make_unique<GroupButton>();

        auto back_icon = std::make_unique<Icon>();
        back_icon->set_icon_name("go-previous");
        back_icon->set_icon_size(16);
        navigation->add_item(std::move(back_icon));

        auto forward_icon = std::make_unique<Icon>();
        forward_icon->set_icon_name("go-next");
        forward_icon->set_icon_size(16);
        navigation->add_item(std::move(forward_icon));

        navigation->set_fixed_size(80);
        m_navigation = navigation.get();
        add_child(std::move(navigation));

        // Home Button (Show All)
        auto home_btn = std::make_unique<GroupButton>();
        home_btn->add_item(i18n().tr("preferences.toolbar.show_all"));
        home_btn->set_fixed_size(100);
        m_home_button = home_btn.get();
        add_child(std::move(home_btn));

        add_child(Spacer());

        // Search Box
        auto search_box = std::make_unique<SearchBox>();
        search_box->set_placeholder(i18n().tr("preferences.toolbar.search_placeholder"));
        search_box->set_fixed_size(220);
        m_search_box = search_box.get();
        add_child(std::move(search_box));
    }
} // namespace horizon::preferences
