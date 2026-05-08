#include "horizon/GraphicsContext.hpp"
#include <horizon-network/WirelessDevice.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Frame.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Label.hpp>
#include <horizon/Notebook.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/TextBoxPolicies.hpp>
#include <views/NetworkView/AdvancedNetworkView.hpp>

namespace horizon::preferences
{
    AdvancedNetworkView::AdvancedNetworkView(const network::DeviceDetails &device)
        : Widget(), m_device(device)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(0);
        set_margin(0);

        setup_ui();
    }

    void AdvancedNetworkView::setup_ui()
    {
        auto notebook = std::make_unique<Notebook>();

        // --- Tab 1: General ---
        notebook->add_tab(NotebookPage("General", create_general_tab()));

        // --- Tab 2: Wifi (Conditional) ---
        if (m_device.type == network::DeviceType::Wifi)
        {
            notebook->add_tab(NotebookPage("Wi-Fi", create_wifi_tab()));
        }

        // --- Tab 3: TCP/IP ---
        notebook->add_tab(NotebookPage("TCP/IP", create_tcpip_tab()));

        m_notebook = notebook.get();
        add_child(std::move(notebook));
    }

    std::unique_ptr<Widget> AdvancedNetworkView::create_general_tab()
    {
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(20);
        container->set_spacing(15);

        auto add_info = [&](const std::string &label, const std::string &value)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(25);
            auto lbl = std::make_unique<Label>(label);
            lbl->set_alignment(TextAlignment::Right);
            lbl->set_font_weight(FONT_WEIGHT_BOLD);
            row->add_child(std::move(lbl));
            row->add_child(Spacer(10));
            auto val = std::make_unique<Label>(value);
            val->set_margin(10);
            row->add_child(std::move(val));
            container->add_child(std::move(row));
        };

        add_info("Interface:", m_device.name);
        add_info("Connection:", m_device.connection_name);
        add_info("Status:", m_device.status_text);
        add_info("Type:", m_device.type == network::DeviceType::Wifi ? "Wireless" : "Wired");

        return container;
    }

    std::unique_ptr<Widget> AdvancedNetworkView::create_wifi_tab()
    {
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(20);
        container->set_spacing(15);

        auto create_input =
            [&](const std::string &label, TextBoxBase **out_ptr, bool password = false)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            auto lbl = std::make_unique<Label>(label);
            lbl->set_alignment(TextAlignment::Right);
            row->add_child(std::move(lbl));

            row->add_child(Spacer(10));

            std::unique_ptr<TextBoxBase> input;
            if (password)
                input = std::make_unique<TextBox<PasswordPolicy>>();
            else
                input = std::make_unique<TextBox<TextPolicy>>();

            input->set_fixed_size(-1);
            *out_ptr = input.get();
            row->add_child(std::move(input));
            container->add_child(std::move(row));
        };

        create_input("SSID:", &m_ssid_input);
        if (m_ssid_input)
            m_ssid_input->set_text(m_device.connection_name);

        auto security_row = std::make_unique<Widget>();
        security_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        security_row->set_fixed_size(35);
        auto sec_lbl = std::make_unique<Label>("Security:");
        sec_lbl->set_alignment(TextAlignment::Right);
        security_row->add_child(std::move(sec_lbl));

        security_row->add_child(Spacer(10));

        auto combo = std::make_unique<Combo>();
        combo->add_item("none", "None");
        combo->add_item("wpa2", "WPA2 Personal");
        combo->add_item("wpa3", "WPA3 Personal");
        combo->set_width(200);
        m_security_combo = combo.get();
        security_row->add_child(std::move(combo));
        container->add_child(std::move(security_row));

        create_input("Password:", &m_password_input, true);

        container->add_child(Spacer());

        // Connect button for Wifi
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(40);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto connect_btn = std::make_unique<Button<AquaObject>>();
        connect_btn->set_text("Connect");
        connect_btn->set_fixed_size(100);
        connect_btn->set_accent_color(WidgetAccentColor::Primary);
        connect_btn->when_click.connect([this](MouseButtonEventContext &)
                                        { this->on_connect_wifi_clicked(); });
        buttons->add_child(std::move(connect_btn));

        container->add_child(std::move(buttons));

        return container;
    }

    std::unique_ptr<Widget> AdvancedNetworkView::create_tcpip_tab()
    {
        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(20);
        container->set_spacing(15);

        auto method_row = std::make_unique<Widget>();
        method_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        method_row->set_fixed_size(35);
        auto m_lbl = std::make_unique<Label>("Configure IPv4:");
        m_lbl->set_alignment(TextAlignment::Right);
        method_row->add_child(std::move(m_lbl));

        method_row->add_child(Spacer(10));

        auto combo = std::make_unique<Combo>();
        combo->add_item("dhcp", "Using DHCP");
        combo->add_item("manual", "Manually");
        combo->set_width(200);
        combo->set_selected_item_by_id(m_device.config_method == "Manual" ? "manual" : "dhcp");
        combo->when_item_selected.connect([this](const ComboItemSelectedContext &)
                                          { this->update_tcpip_fields_visibility(); });
        m_ipv4_method_combo = combo.get();
        method_row->add_child(std::move(combo));
        container->add_child(std::move(method_row));

        auto create_input =
            [&](const std::string &label, TextBoxBase **out_ptr, const std::string &val)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            auto lbl = std::make_unique<Label>(label);
            lbl->set_alignment(TextAlignment::Right);
            row->add_child(std::move(lbl));

            row->add_child(Spacer(10));

            auto input = std::make_unique<TextBox<TextPolicy>>();
            input->set_fixed_size(-1);
            input->set_text(val == "---" ? "" : val);
            *out_ptr = input.get();
            row->add_child(std::move(input));
            container->add_child(std::move(row));
        };

        create_input("IP Address:", &m_ip_input, m_device.ip_address);
        create_input("Subnet Mask:", &m_mask_input, m_device.subnet_mask);
        create_input("Router:", &m_router_input, m_device.router);
        create_input("DNS Server:", &m_dns_input, m_device.dns);

        update_tcpip_fields_visibility();

        container->add_child(Spacer());

        // Bottom Apply button INSIDE TCP/IP tab
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(40);
        buttons->set_spacing(10);
        buttons->add_child(Spacer());

        auto apply_btn = std::make_unique<Button<AquaObject>>();
        apply_btn->set_text("Apply");
        apply_btn->set_fixed_size(100);
        apply_btn->set_accent_color(WidgetAccentColor::Primary);
        apply_btn->when_click.connect([this](MouseButtonEventContext &)
                                      { this->on_apply_clicked(); });
        buttons->add_child(std::move(apply_btn));

        container->add_child(std::move(buttons));

        return container;
    }

    void AdvancedNetworkView::update_tcpip_fields_visibility()
    {
        bool manual = false;
        if (m_ipv4_method_combo && m_ipv4_method_combo->selected_item())
        {
            manual = (m_ipv4_method_combo->selected_item()->id == "manual");
        }

        if (m_ip_input)
            m_ip_input->set_enabled(manual);
        if (m_mask_input)
            m_mask_input->set_enabled(manual);
        if (m_router_input)
            m_router_input->set_enabled(manual);
        if (m_dns_input)
            m_dns_input->set_enabled(manual);
    }

    void AdvancedNetworkView::on_apply_clicked()
    {
        network::DeviceDetails updated = m_device;
        if (m_ipv4_method_combo && m_ipv4_method_combo->selected_item())
        {
            updated.config_method =
                (m_ipv4_method_combo->selected_item()->id == "manual" ? "Manual" : "DHCP");
        }
        if (m_ip_input)
            updated.ip_address = m_ip_input->text();
        if (m_mask_input)
            updated.subnet_mask = m_mask_input->text();
        if (m_router_input)
            updated.router = m_router_input->text();
        if (m_dns_input)
            updated.dns = m_dns_input->text();

        if (network::NetworkManager::instance().apply_device_settings(updated))
        {
            // Successfully applied
        }
    }

    void AdvancedNetworkView::on_connect_wifi_clicked()
    {
        if (m_device.type != network::DeviceType::Wifi)
            return;

        std::string ssid = m_ssid_input ? m_ssid_input->text() : "";
        std::string password = m_password_input ? m_password_input->text() : "";

        // For now we assume we are using the device from m_device.path
        auto dev = std::make_shared<network::WirelessDevice>(m_device.name, m_device.path);
        dev->connect(ssid, password, ""); // ap_path empty for manual entry? Or find AP.
    }
} // namespace horizon::preferences
