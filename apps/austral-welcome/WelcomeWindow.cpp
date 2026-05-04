#include "WelcomeWindow.hpp"
#include "horizon/AirObject.hpp"
#include <cstdlib>
#include <filesystem>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Color.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    static std::string get_config_path(const std::string &filename)
    {
        const char *home = std::getenv("HOME");
        if (!home)
            return filename;

        std::filesystem::path p(home);
        p /= ".config/horizon";
        p /= filename;
        return p.string();
    }

    WelcomeWindow::WelcomeWindow() : Window(i18n().tr("welcome.title"))
    {
        m_config = std::make_unique<ConfigManager>(get_config_path("austral-welcome.json"));
        m_config->load();

        setup_ui();
        load_config();
    }

    void WelcomeWindow::setup_ui()
    {
        auto main_container = std::make_unique<Widget>();
        main_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        main_container->set_margin(20);

        // Content Area (Icon + Text)
        auto content_area = std::make_unique<Widget>();
        content_area->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        content_area->set_position_type(FILL);

        // Left: Icon
        auto icon_container = std::make_unique<Widget>();
        icon_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        icon_container->set_fixed_size(300); // Approximate width for icon area

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name("emblem-austral");
        icon->set_icon_size(192);
        icon->set_size(192, 192);

        icon_container->add_child(Spacer());
        icon_container->add_child(std::move(icon));
        icon_container->add_child(Spacer());

        // Right: Text
        auto text_container = std::make_unique<Widget>();
        text_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        text_container->set_position_type(FILL);

        auto title = std::make_unique<Label>(i18n().tr("welcome.title_long"));
        title->set_font_size(36);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_margin(20);

        auto desc = std::make_unique<Label>(i18n().tr("welcome.description"));
        desc->set_font_size(16);
        desc->set_text_color(Color(0.3f, 0.3f, 0.3f)); // Slightly dimmed text

        text_container->add_child(Spacer());
        text_container->add_child(std::move(title));
        text_container->add_child(std::move(desc));
        text_container->add_child(Spacer());

        content_area->add_child(std::move(icon_container));
        content_area->add_child(std::move(text_container));

        // Footer Area
        auto footer_area = std::make_unique<Widget>();
        footer_area->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        footer_area->set_fixed_size(30);

        auto start_on_boot = std::make_unique<Checkbox<AquaObject>>();
        start_on_boot->set_text(i18n().tr("welcome.start_on_boot"));
        start_on_boot->set_fixed_size(300);
        start_on_boot->set_checked(true);
        m_start_on_boot_check = start_on_boot.get();

        m_start_on_boot_check->when_toggle.connect(
            [this](const ToggleEventContext &ctx)
            {
                m_show_welcome = ctx.checked;
                save_config();
            });

        auto btn_next = std::make_unique<Button<AirObject>>();
        btn_next->set_text(i18n().tr("welcome.buttons.ok"));
        btn_next->set_fixed_size(120);
        btn_next->when_click.connect([this](const MouseButtonEventContext &) {
            application()->quit();
        });

        footer_area->add_child(std::move(start_on_boot));
        footer_area->add_child(Spacer());
        footer_area->add_child(std::move(btn_next));

        main_container->add_child(std::move(content_area));
        main_container->add_child(std::move(footer_area));

        add_child(std::move(main_container));
    }

    void WelcomeWindow::load_config()
    {
        auto section = m_config->get_section("welcome");
        m_show_welcome = section.value("show_welcome", true);
        if (m_start_on_boot_check)
        {
            m_start_on_boot_check->set_checked(m_show_welcome);
        }
    }

    void WelcomeWindow::save_config()
    {
        nlohmann::json j;
        j["show_welcome"] = m_show_welcome;
        m_config->set_section("welcome", j);
        m_config->save();
    }
} // namespace horizon
