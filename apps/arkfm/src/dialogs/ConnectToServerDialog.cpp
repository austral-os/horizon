#include "dialogs/ConnectToServerDialog.hpp"
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>

namespace horizon::arkfm
{
    ConnectToServerDialog::ConnectToServerDialog() : WaylandWindow("arkfm.dialog", 450, 220)
    {
        set_name("Conectar al servidor");

        auto window_widget = std::make_unique<horizon::Window>("Conectar al servidor");

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_spacing(15);
        content->set_margin(20);

        auto label = std::make_unique<Label>("Ingrese la dirección del servidor:");
        content->add_child(std::move(label));

        auto input_row = std::make_unique<Widget>();
        input_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        input_row->set_spacing(10);
        input_row->set_fixed_size(30);

        auto combo = std::make_unique<Combo>();
        combo->set_fixed_size(200);
        combo->add_item("smb://", "Windows (SMB)");
        combo->add_item("ftp://", "FTP");
        combo->add_item("sftp://", "SFTP");
        combo->add_item("dav://", "WebDAV");
        m_protocol_combo = combo.get();

        auto address = std::make_unique<TextBox<>>();
        address->set_placeholder("192.168.1.100/share");
        address->set_fixed_size(-1);
        m_address_input = address.get();

        input_row->add_child(std::move(combo));
        input_row->add_child(std::move(address));
        content->add_child(std::move(input_row));

        content->add_child(horizon::Spacer());

        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_spacing(10);
        buttons->set_fixed_size(30);
        buttons->add_child(horizon::Spacer());

        auto cancel = std::make_unique<Button<AquaObject>>();
        cancel->set_text("Cancelar");
        cancel->set_width(100);
        cancel->when_click.connect([this](auto &) { quit(); });

        auto connect = std::make_unique<Button<AquaObject>>();
        connect->set_text("Conectar");
        connect->set_accent_color(WidgetAccentColor::Primary);
        connect->set_width(100);
        connect->when_click.connect([this](auto &) { handle_connect(); });

        buttons->add_child(std::move(cancel));
        buttons->add_child(std::move(connect));
        content->add_child(std::move(buttons));

        window_widget->add_child(std::move(content));
        set_root(std::move(window_widget));
    }

    void ConnectToServerDialog::handle_connect()
    {
        if (!m_protocol_combo || !m_address_input)
            return;

        const auto *item = m_protocol_combo->selected_item();
        if (!item)
            return;

        std::string uri = item->id + m_address_input->text();
        if (m_address_input->text().empty())
            return;

        ConnectToServerEvent ev;
        ev.uri = uri;
        when_accepted.run(ev);
        quit();
    }
} // namespace horizon::arkfm
