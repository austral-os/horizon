#include "InstallPage.hpp"
#include "horizon/Icon.hpp"
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/ToolbarButton.hpp>

namespace horizon::installer
{
    InstallPage::InstallPage()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(40);

        auto logo = std::make_unique<Icon>();
        logo->set_icon_name("emblem-austral");
        logo->set_icon_size(256);
        logo->set_size(256, 256);

        auto logo_container = std::make_unique<Widget>();
        logo_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        logo_container->add_child(Spacer());
        logo_container->add_child(std::move(logo));
        logo_container->add_child(Spacer());

        add_child(Spacer());
        add_child(std::move(logo_container));

        auto title = std::make_unique<Label>(i18n().tr("installer.install.title"));
        title->set_font_size(32);
        title->set_alignment(TextAlignment::Center);
        add_child(std::move(title));

        auto progress = std::make_unique<ProgressBar>();
        m_progress = progress.get();
        add_child(std::move(progress));
        m_progress->set_fixed_size(30);

        auto status = std::make_unique<Label>(i18n().tr("installer.install.desc"));
        m_status = status.get();
        m_status->set_fixed_size(40);
        add_child(std::move(status));
        m_status->set_alignment(TextAlignment::Center);

        add_child(Spacer(50));

        auto btn_cancel =
            std::make_unique<ToolbarButton>(i18n().tr("installer.buttons.cancel"), "process-stop");
        btn_cancel->when_click.connect(
            [this](auto &)
            {
                EventContext ctx;
                when_cancel.run(ctx);
            });

        auto btn_container = std::make_unique<Widget>();
        btn_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btn_container->add_child(Spacer());
        btn_container->add_child(std::move(btn_cancel));
        btn_container->add_child(Spacer());
        btn_container->set_fixed_size(70);
        add_child(std::move(btn_container));
    }

    void InstallPage::update_progress(float progress, const std::string &message)
    {
        m_progress->set_progress(progress);
        m_status->set_text(message);
    }
} // namespace horizon::installer
