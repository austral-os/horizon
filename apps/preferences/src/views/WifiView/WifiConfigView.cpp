#include <algorithm>
#include <cstdio>
#include <map>
#include <horizon/Icon.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/WaylandWindow.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <views/WifiView/WifiConfigView.hpp>
#include <views/WifiView/WifiConnectDialog.hpp>
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    WifiConfigView::WifiConfigView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(10);
        set_margin(10);

        // Get the first wireless device available
        auto devices = network::NetworkManager::instance().get_wireless_devices();
        if (!devices.empty())
        {
            m_device = devices[0];
        }

        setup_ui();
        refresh_networks();

        when_application_load.connect(
            [this](EventContext &)
            {
                if (auto *app = application())
                {
                    app->add_timer(1000, [this]() { m_initialized = true; });
                }
            });

        // Listen for network changes from the library
        m_state_changed_connection_id = network::NetworkManager::instance().when_state_changed.connect(
            [this, alive = m_alive](EventContext &)
            {
                if (!alive->load())
                    return;

                if (auto *app = WaylandWindow::get_active_window())
                {
                    app->post_task([this, alive]()
                    {
                        if (alive->load())
                            this->refresh_networks();
                    });
                }
            });
    }

    WifiConfigView::~WifiConfigView()
    {
        m_alive->store(false);

        if (m_refresh_timer_id != 0)
        {
            if (auto *app = application())
            {
                app->stop_timer(m_refresh_timer_id);
            }
        }

        if (m_state_changed_connection_id != 0)
        {
            network::NetworkManager::instance().when_state_changed.disconnect(m_state_changed_connection_id);
        }
    }

    void WifiConfigView::setup_ui()
    {
        auto label = std::make_unique<Label>(i18n().tr("preferences.wifi.preferred_networks"));
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_fixed_size(24);
        m_title_label = label.get();
        add_child(std::move(label));

        auto table = std::make_unique<TableView<network::WifiNetwork>>();
        table->set_height(250);

        TableColumn<network::WifiNetwork> ssid_col;
        ssid_col.title = i18n().tr("preferences.wifi.network_name");
        ssid_col.width = 250;
        ssid_col.cell_factory = [](const network::WifiNetwork &data)
        { return std::make_unique<Label>(data.ssid); };
        table->add_column(std::move(ssid_col));

        TableColumn<network::WifiNetwork> security_col;
        security_col.title = i18n().tr("preferences.wifi.security_type");
        security_col.width = 150;
        security_col.cell_factory = [](const network::WifiNetwork &data)
        { return std::make_unique<Label>(data.security); };
        table->add_column(std::move(security_col));

        TableColumn<network::WifiNetwork> signal_col;
        signal_col.title = i18n().tr("preferences.wifi.signal");
        signal_col.width = 70;
        signal_col.cell_factory = [](const network::WifiNetwork &data)
        { return std::make_unique<Label>(std::to_string(data.signal) + "%"); };
        table->add_column(std::move(signal_col));

        TableColumn<network::WifiNetwork> connection_col;
        connection_col.title = i18n().tr("preferences.wifi.connection");
        connection_col.width = 150;
        connection_col.cell_factory = [](const network::WifiNetwork &data)
        {
            auto lbl = std::make_unique<Label>(data.connected ? i18n().tr("preferences.wifi.connected") : "");
            if (data.connected)
            {
                lbl->set_text_color(Color("#0b7c37ff"));
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
            }
            return lbl;
        };
        table->add_column(std::move(connection_col));

        m_table_view = table.get();
        m_table_view->when_row_click.connect([this](auto &ctx)
                                             { this->on_network_selected(ctx.row_data); });
        add_child(std::move(table));

        auto button_container = std::make_unique<Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(35);
        button_container->set_spacing(10);

        auto connect_btn = std::make_unique<Button<SolidObject>>();
        connect_btn->set_text(i18n().tr("preferences.wifi.connect"));
        connect_btn->set_enabled(false);
        connect_btn->when_click.connect([this](MouseButtonEventContext &) { this->on_connect_clicked(); });

        m_connect_button = connect_btn.get();
        button_container->add_child(std::move(connect_btn));

        auto refresh_btn = std::make_unique<Button<SolidObject>>();
        refresh_btn->set_text(i18n().tr("preferences.wifi.refresh"));
        refresh_btn->when_click.connect([this](MouseButtonEventContext &)
                                        { this->refresh_networks(); });
        m_refresh_button = refresh_btn.get();
        button_container->add_child(std::move(refresh_btn));

        button_container->add_child(Spacer());
        add_child(std::move(button_container));

        auto options_container = std::make_unique<Widget>();
        options_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        options_container->set_fixed_size(24);

        auto status_label = std::make_unique<Label>(i18n().tr("preferences.wifi.no_connection"));
        status_label->set_alignment(TextAlignment::Center);
        m_active_network_label = status_label.get();
        options_container->add_child(std::move(status_label));

        add_child(std::move(options_container));
    }

    void WifiConfigView::refresh_networks()
    {
        if (m_table_view && m_device)
        {
            m_device->request_scan();
            auto networks = m_device->get_access_points();
            m_table_view->set_data(networks);

            std::string active_ssid = m_device->get_active_ssid();
            if (!active_ssid.empty())
            {
                if (m_active_network_label)
                {
                    m_active_network_label->set_text(i18n().tr("preferences.wifi.connected_to", {{"0", active_ssid}}));
                    m_active_network_label->set_text_color(Color("#2ecc71"));
                    m_active_network_label->set_font_weight(FONT_WEIGHT_BOLD);
                }
            }
            else
            {
                if (m_active_network_label)
                {
                    m_active_network_label->set_text(i18n().tr("preferences.wifi.no_connection"));
                    m_active_network_label->set_text_color(Color("#000000"));
                    m_active_network_label->set_font_weight(FONT_WEIGHT_NORMAL);
                }
            }

            if (auto *app = application())
            {
                if (m_refresh_timer_id != 0) app->stop_timer(m_refresh_timer_id);
                m_refresh_timer_id = app->add_timer(5000, [this]() {
                    m_refresh_timer_id = 0;
                    this->refresh_networks();
                });
            }
        }
    }

    void WifiConfigView::on_network_selected(const network::WifiNetwork &network)
    {
        if (!m_initialized) return;
        m_selected_network = network;
        if (m_connect_button)
        {
            m_connect_button->set_enabled(true);
            m_connect_button->set_text(network.connected ? i18n().tr("preferences.wifi.disconnect") : i18n().tr("preferences.wifi.connect"));
        }
    }

    void WifiConfigView::on_connect_clicked()
    {
        if (m_selected_network.ssid.empty()) return;
        if (m_selected_network.connected)
        {
            disconnect_selected();
            return;
        }

        if (m_dialog_open || !m_device) return;

        if (auto *app = application())
        {
            m_dialog_open = true;
            auto dialog = std::make_unique<WifiConnectDialog>(m_device, m_selected_network);
            dialog->when_close.connect([this](EventContext &) { m_dialog_open = false; });

            std::thread([this, app, d = std::move(dialog), alive = m_alive]() mutable {
                d->initialize();
                d->run();
                app->post_task([this, alive]() {
                    if (alive->load())
                        m_dialog_open = false;
                });
            }).detach();
        }
    }

    void WifiConfigView::disconnect_selected()
    {
        if (m_selected_network.ssid.empty() || !m_device) return;
        m_device->disconnect(m_selected_network.ssid);
    }
}
