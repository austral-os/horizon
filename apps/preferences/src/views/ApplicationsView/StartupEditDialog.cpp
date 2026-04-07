#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Window.hpp>
#include <views/ApplicationsView/StartupEditDialog.hpp>

namespace horizon::preferences
{
    StartupEditDialog::StartupEditDialog(const DesktopEntry& entry)
        : WaylandWindow("horizon.startup_edit", 450, 250, true, true)
    {
        set_name("Editar Comando de Inicio");
        setup_ui(entry);
    }

    void StartupEditDialog::setup_ui(const DesktopEntry& entry)
    {
        auto root_wnd = std::make_unique<Window>(name());
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_wnd->set_margin(0);
        root_wnd->set_spacing(0);

        auto container = std::make_unique<Widget>();
        container->set_margin(20);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(15);

        // Header: Icon and Name
        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_spacing(15);
        header->set_fixed_size(48);

        auto icon = std::make_unique<Icon>();
        icon->set_icon_name(entry.icon.empty() ? "application-x-executable" : entry.icon);
        icon->set_icon_size(48);
        icon->set_fixed_size(48);
        header->add_child(std::move(icon));

        auto name_label = std::make_unique<Label>(entry.name);
        name_label->set_font_size(18);
        name_label->set_position_type(WidgetPositionTypes::FILL);
        header->add_child(std::move(name_label));

        container->add_child(std::move(header));

        // Command Input
        auto cmd_label = std::make_unique<Label>("Comando a ejecutar:");
        cmd_label->set_fixed_size(20);
        container->add_child(std::move(cmd_label));

        auto input = std::make_unique<TextBox<TextPolicy>>();
        m_command_input = input.get();
        m_command_input->set_text(entry.exec);
        m_command_input->set_fixed_size(35);
        m_command_input->set_focusable(true);
        container->add_child(std::move(input));

        // Buttons
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(35);
        buttons->set_spacing(10);

        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text("Cancelar");
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext&) {
            EventContext ctx;
            when_cancelled.run(ctx);
            this->quit();
        });
        buttons->add_child(std::move(btn_cancel));

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text("Aceptar");
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);
        btn_accept->when_click.connect([this](MouseButtonEventContext&) {
            std::string val = m_command_input->text();
            when_accepted.run(val);
            this->quit();
        });
        buttons->add_child(std::move(btn_accept));

        container->add_child(std::move(buttons));

        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }
} // namespace horizon::preferences
