#include "SuccessPage.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ToolbarButton.hpp>

namespace horizon::installer
{
    SuccessPage::SuccessPage(bool is_oobe)
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

        auto title = std::make_unique<Label>(i18n().tr("installer.success.title"));
        title->set_font_size(48);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto desc = std::make_unique<Label>(i18n().tr("installer.success.desc"));
        desc->set_font_size(18);
        desc->set_text_color(Color(0.4f, 0.4f, 0.4f));
        desc->set_alignment(TextAlignment::Center);
        add_child(std::move(desc));

        if (!is_oobe)
        {
            add_child(Spacer(20));

            auto warning = std::make_unique<Label>(i18n().tr("installer.success.remove_medium"));
            warning->set_font_size(22);
            warning->set_text_color(Color(1.0f, 0.3f, 0.3f)); // High contrast red
            warning->set_alignment(TextAlignment::Center);
            add_child(std::move(warning));
        }

        add_child(Spacer());

        auto btn_finish = std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.restart"),
                                                          "system-reboot");
        btn_finish->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_finish.run(ctx);
            });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_finish));
        btn_container->add_child(Spacer());
        btn_container->set_fixed_size(70);
        add_child(std::move(btn_container));
    }
} // namespace horizon::installer
