#include "horizon/Widget.hpp"
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Frame.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableView.hpp>
#include <views/NetworkView/NetworkView.hpp>

namespace horizon::preferences
{
    NetworkView::NetworkView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(20);
        set_margin(20);

        setup_ui();
        refresh_devices();

        network::NetworkManager::instance().when_state_changed.connect(
            [this](EventContext &) { this->refresh_devices(); });
    }

    void NetworkView::setup_ui()
    {
        const int left_panel_width = 230;

        // --- Left Panel: TableView ---
        auto left_panel = std::make_unique<Widget>();
        left_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        left_panel->set_fixed_size(left_panel_width);
        left_panel->set_spacing(10);

        auto table = std::make_unique<TableView<network::DeviceDetails>>();
        table->set_header_visible(false);
        table->set_row_height(60);
        table->set_background_color(Color("#f0f0f0"));

        TableColumn<network::DeviceDetails> col;
        col.width = left_panel_width;
        col.cell_factory = [](const network::DeviceDetails &data)
        {
            auto container = std::make_unique<Widget>();
            container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            container->set_spacing(12);
            container->set_margin(8);

            // 1. Status Dot
            auto dot = std::make_unique<SolidObject>();
            dot->set_fixed_size(12);
            dot->set_border_radius(6);
            dot->set_background_color(data.connected ? Color("#2ecc71") : Color("#e74c3c"));
            container->add_child(std::move(dot));

            // 2. Info Container
            auto info = std::make_unique<Widget>();
            info->set_layout_type(WIDGET_LAYOUT_VERTICAL);
            info->set_spacing(2);

            auto title = std::make_unique<Label>(
                data.type == network::DeviceType::Wifi ? "Wi-Fi" : "Ethernet");
            title->set_font_weight(FONT_WEIGHT_BOLD);
            info->add_child(std::move(title));

            auto subtitle = std::make_unique<Label>(
                data.connected ? data.ip_address : i18n().tr("preferences.network.not_connected"));
            subtitle->set_text_color(Color("#666666"));
            // subtitle->set_font_size(12);
            info->add_child(std::move(subtitle));

            container->add_child(std::move(info));

            // 3. Icon
            container->add_child(Spacer());
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(data.type == network::DeviceType::Wifi ? "network-wireless"
                                                                       : "network-wired");
            icon->set_fixed_size(24);
            container->add_child(std::move(icon));

            return container;
        };
        table->add_column(std::move(col));

        m_device_table = table.get();
        m_device_table->when_row_click.connect([this](auto &ctx)
                                               { this->on_device_selected(ctx.row_data); });

        left_panel->add_child(std::move(table));

        // Bottom buttons (+ / - / Gear)
        auto toolbar = std::make_unique<Widget>();
        toolbar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        toolbar->set_fixed_size(30);
        toolbar->set_spacing(2);

        auto add_btn = std::make_unique<Button<SolidObject>>();
        add_btn->set_text("+");
        add_btn->set_fixed_size(30);
        toolbar->add_child(std::move(add_btn));

        auto rem_btn = std::make_unique<Button<SolidObject>>();
        rem_btn->set_text("-");
        rem_btn->set_fixed_size(30);
        toolbar->add_child(std::move(rem_btn));

        auto settings_btn = std::make_unique<Button<SolidObject>>();
        settings_btn->set_text("⚙");
        settings_btn->set_fixed_size(30);
        toolbar->add_child(std::move(settings_btn));

        left_panel->add_child(std::move(toolbar));
        add_child(std::move(left_panel));

        // --- Right Panel: Details Frame ---
        auto details = std::make_unique<Frame>();
        details->set_position_type(WidgetPositionTypes::FILL);
        details->set_margin(0);
        details->set_spacing(20);

        auto details_container = std::make_unique<Widget>();
        details_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        details_container->set_margin(20);
        details_container->set_spacing(20);

        auto create_row = [&](const std::string &label_text, Widget *field)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_fixed_size(35);
            row->set_spacing(15);

            auto lbl = std::make_unique<Label>(label_text);
            lbl->set_alignment(TextAlignment::Right);
            lbl->set_width(120);
            row->add_child(std::move(lbl));

            std::unique_ptr<Widget> field_ptr(field);
            row->add_child(std::move(field_ptr));
            return row;
        };

        // Grid-like layout for info
        auto grid = std::make_unique<Widget>();
        grid->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        grid->set_spacing(15);

        /////////////////
        auto status_lbl = std::make_unique<Label>("---");
        m_status_label = status_lbl.get();
        grid->add_child(create_row("Status:", status_lbl.release()));
        ////////////////

        auto desc_lbl = std::make_unique<Label>("");
        desc_lbl->set_alignment(TextAlignment::Center);
        desc_lbl->set_vertical_alignment(VerticalAlignment::Top);
        desc_lbl->set_text_color(Color("#555555"));
        desc_lbl->set_fixed_size(50);
        m_description_label = desc_lbl.get();
        details_container->add_child(std::move(desc_lbl));

        auto config_combo = std::make_unique<Combo>();
        config_combo->add_item("dhcp", "Using DHCP");
        config_combo->add_item("manual", "Manually");
        config_combo->set_width(200);
        m_config_combo = config_combo.get();
        grid->add_child(create_row("Configure IPv4:", config_combo.release()));

        auto ip_val = std::make_unique<Label>("---");
        m_ip_label = ip_val.get();
        grid->add_child(create_row("IP Address:", ip_val.release()));

        auto mask_val = std::make_unique<Label>("---");
        m_mask_label = mask_val.get();
        grid->add_child(create_row("Subnet Mask:", mask_val.release()));

        auto router_val = std::make_unique<Label>("---");
        m_router_label = router_val.get();
        grid->add_child(create_row("Router:", router_val.release()));

        auto dns_val = std::make_unique<Label>("---");
        m_dns_label = dns_val.get();
        grid->add_child(create_row("DNS Server:", dns_val.release()));

        auto domains_val = std::make_unique<Label>("---");
        m_domains_label = domains_val.get();
        grid->add_child(create_row("Search Domains:", domains_val.release()));

        details_container->add_child(std::move(grid));

        // details->add_child(Spacer());

        // Advanced button
        auto bottom_row = std::make_unique<Widget>();
        bottom_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_row->set_fixed_size(40);
        bottom_row->add_child(Spacer());
        auto adv_btn = std::make_unique<Button<AquaObject>>();
        adv_btn->set_text("Advanced...");
        adv_btn->set_width(120);
        bottom_row->add_child(std::move(adv_btn));
        details_container->add_child(std::move(bottom_row));

        m_details_container = details_container.get();

        details->add_child(std::move(details_container));

        add_child(std::move(details));
    }

    void NetworkView::refresh_devices()
    {
        if (m_device_table)
        {
            auto devices = network::NetworkManager::instance().get_all_devices();
            m_device_table->set_data(devices);

            if (!devices.empty())
            {
                // Select first by default if nothing selected?
                // For now we wait for click.
            }
        }
    }

    void NetworkView::on_device_selected(const network::DeviceDetails &dev)
    {
        if (m_status_label)
        {
            m_status_label->set_text(dev.connected ? "Connected" : "Not Connected");
            m_status_label->set_text_color(dev.connected ? Color("#2ecc71") : Color("#e74c3c"));
        }

        if (m_description_label)
        {
            if (dev.connected)
            {
                std::string type_name =
                    (dev.type == network::DeviceType::Wifi ? "Wi-Fi" : "Ethernet");
                m_description_label->set_text(type_name +
                                              " is currently active and has the IP address " +
                                              dev.ip_address + ".");
            }
            else
            {
                m_description_label->set_text("This device is not connected.");
            }
        }

        if (m_ip_label)
            m_ip_label->set_text(dev.connected ? dev.ip_address : "---");
        if (m_mask_label)
            m_mask_label->set_text(dev.connected ? dev.subnet_mask : "---");
        if (m_router_label)
            m_router_label->set_text(dev.connected ? dev.router : "---");
        if (m_dns_label)
            m_dns_label->set_text(dev.connected ? dev.dns : "---");
        if (m_domains_label)
            m_domains_label->set_text(dev.connected ? dev.search_domains : "---");

        if (m_config_combo)
        {
            m_config_combo->set_selected_item_by_id(dev.connected ? "dhcp" : "");
        }
    }
} // namespace horizon::preferences
