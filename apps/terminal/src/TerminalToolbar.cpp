#include "TerminalToolbar.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <memory>

namespace horizon::terminal
{

    TerminalToolbar::TerminalToolbar() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(5);
        set_spacing(10);

        // 1. New Tab Button (Left)
        auto new_tab_group = std::make_unique<horizon::GroupButton>();
        m_new_tab_group = new_tab_group.get();
        m_new_tab_group->set_fixed_size(40);

        auto add_icon = std::make_unique<horizon::Icon>();
        add_icon->set_icon_name("list-add");
        add_icon->set_icon_size(16);
        add_icon->set_use_theme_colors(true);
        m_new_tab_group->add_item(std::move(add_icon));

        m_new_tab_group->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &)
            {
                NewTabClickEvent ev;
                this->when_new_tab_clicked.run(ev);
            });

        // 2. Spacer (Middle)
        auto spacer = horizon::Spacer();
        spacer->set_position_type(FILL);

        // 3. Right side buttons (Fullscreen / Preferences)
        auto settings_group = std::make_unique<horizon::GroupButton>();
        m_settings_group = settings_group.get();
        m_settings_group->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        m_settings_group->set_fixed_size(80);

        auto fullscreen_icon = std::make_unique<horizon::Icon>();
        fullscreen_icon->set_icon_name("view-fullscreen");
        fullscreen_icon->set_icon_size(16);
        fullscreen_icon->set_use_theme_colors(true);
        m_settings_group->add_item(std::move(fullscreen_icon));

        auto preferences_icon = std::make_unique<horizon::Icon>();
        preferences_icon->set_icon_name("emblem-system");
        preferences_icon->set_icon_size(16);
        preferences_icon->set_use_theme_colors(true);
        m_settings_group->add_item(std::move(preferences_icon));

        m_settings_group->when_button_clicked.connect(
            [this](horizon::GroupButtonClickEvent &ctx)
            {
                if (ctx.button_index == 0)
                {
                    FullscreenClickEvent ev;
                    this->when_fullscreen_clicked.run(ev);
                }
                else
                {
                    PreferencesClickEvent ev;
                    this->when_preferences_clicked.run(ev);
                }
            });

        add_child(std::move(new_tab_group));
        add_child(std::move(spacer));
        add_child(std::move(settings_group));
    }

} // namespace horizon::terminal
