#include "WelcomePage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ToolbarButton.hpp>

namespace horizon::installer
{
    WelcomePage::WelcomePage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        add_child(Spacer());

        auto logo = std::make_unique<Icon>();
        logo->set_icon_name("emblem-austral");
        logo->set_icon_size(128);
        logo->set_size(128, 128);

        auto logo_container = std::make_unique<Widget>();
        logo_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        logo_container->add_child(Spacer());
        logo_container->add_child(std::move(logo));
        logo_container->add_child(Spacer());
        add_child(std::move(logo_container));

        auto title = std::make_unique<Label>(i18n().tr("installer.welcome.title"));
        title->set_font_size(48);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.welcome.desc"));
        desc->set_font_size(18);
        desc->set_text_color(Color(0.4f, 0.4f, 0.4f));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        add_child(Spacer());

        auto btn_next =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.continue"), "go-next");
        btn_next->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_continue.run(ctx);
            });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_next));
        btn_container->add_child(Spacer());
        btn_container->set_fixed_size(70);
        add_child(std::move(btn_container));
    }
} // namespace horizon::installer
