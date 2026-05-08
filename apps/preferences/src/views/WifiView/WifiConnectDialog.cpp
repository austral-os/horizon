#include <horizon/AquaObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <iostream>
#include <views/WifiView/WifiConnectDialog.hpp>

namespace horizon::preferences
{
    WifiConnectDialog::WifiConnectDialog(std::shared_ptr<network::WirelessDevice> device, const network::WifiNetwork& network)
        : WaylandWindow("horizon.wifi_connect", 450, 350, true, false), m_device(device), m_network(network)
    {
        set_name("Conectar a " + m_network.ssid);
        setup_ui();
    }

    void WifiConnectDialog::setup_ui()
    {
        auto root_wnd = std::make_unique<Window>("Conectar a " + m_network.ssid);
        root_wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto container = std::make_unique<Widget>();
        container->set_margin(20);
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_spacing(15);

        auto label = std::make_unique<Label>("Introduce la clave para '" + m_network.ssid + "':");
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_fixed_size(25);
        container->add_child(std::move(label));

        auto pass_label = std::make_unique<Label>("Contraseña:");
        pass_label->set_fixed_size(15);
        container->add_child(std::move(pass_label));

        auto input = std::make_unique<TextBox<PasswordPolicy>>();
        m_password_input = input.get();
        m_password_input->set_fixed_size(35);
        m_password_input->set_focusable(true);
        container->add_child(std::move(input));

        auto loading_bar = std::make_unique<LoadingBar>();
        loading_bar->set_fixed_size(24);
        loading_bar->set_visible(false);
        m_loading_bar = loading_bar.get();
        container->add_child(std::move(loading_bar));

        auto status_label = std::make_unique<Label>("No conectado");
        status_label->set_alignment(TextAlignment::Center);
        m_status_label = status_label.get();
        container->add_child(std::move(status_label));

        container->add_child(Spacer());

        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(40);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text("Cancelar");
        btn_cancel->set_fixed_size(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) { this->quit(); });
        m_cancel_btn = btn_cancel.get();
        buttons->add_child(std::move(btn_cancel));

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text("Conectar");
        btn_accept->set_fixed_size(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);
        m_accept_btn = btn_accept.get();

        btn_accept->when_click.connect([this](MouseButtonEventContext &) {
            if (m_accept_btn && m_accept_btn->text() == "Cerrar") {
                this->quit();
                return;
            }

            std::string password = m_password_input ? m_password_input->text() : "";
            
            if (m_status_label) {
                m_status_label->set_text("Conectando...");
                m_status_label->set_text_color(Color("#666666"));
            }
            if (m_loading_bar) m_loading_bar->set_visible(true);
            if (m_accept_btn) {
                m_accept_btn->set_text("Conectando...");
                m_accept_btn->set_enabled(false);
            }
            if (m_password_input) m_password_input->set_enabled(false);

            std::thread([this, password]() { this->perform_connection_async(password); }).detach();
        });
        buttons->add_child(std::move(btn_accept));

        container->add_child(std::move(buttons));
        root_wnd->add_child(std::move(container));
        set_root(std::move(root_wnd));
    }

    void WifiConnectDialog::perform_connection_async(const std::string &password)
    {
        if (!m_device) return;

        bool success = m_device->connect(m_network.ssid, password, m_network.path);

        if (success)
        {
            if (m_status_label) {
                m_status_label->set_text_color(Color("#44aa44"));
                m_status_label->set_text("Conectado exitosamente");
            }
            if (m_accept_btn) {
                m_accept_btn->set_enabled(true);
                m_accept_btn->set_text("Cerrar");
            }
        }
        else
        {
            if (m_status_label) {
                m_status_label->set_text_color(Color("#ff4444"));
                m_status_label->set_text("Fallo en la conexión");
            }
            if (m_accept_btn) {
                m_accept_btn->set_enabled(true);
                m_accept_btn->set_text("Reintentar");
            }
            if (m_password_input) m_password_input->set_enabled(true);
        }
        
        if (m_loading_bar) m_loading_bar->set_visible(false);
    }
}
