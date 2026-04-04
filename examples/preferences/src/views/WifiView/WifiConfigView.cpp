#include <cstdio>
#include <horizon/Icon.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/WaylandWindow.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <views/WifiView/WifiConfigView.hpp>
#include <views/WifiView/WifiConnectDialog.hpp>

namespace horizon::preferences
{
    static std::string get_security_string(uint32_t wpa, uint32_t rsn)
    {
        std::string security = "";
        if (rsn != 0)
            security += "WPA2 ";
        if (wpa != 0)
            security += "WPA ";
        if (security.empty())
            security = "None";
        return security;
    }

    WifiConfigView::WifiConfigView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(10);
        set_margin(10);

        try
        {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to initialize D-Bus: " << e.what() << std::endl;
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
    }

    WifiConfigView::~WifiConfigView()
    {
        if (m_refresh_timer_id != 0)
        {
            if (auto *app = application())
            {
                app->stop_timer(m_refresh_timer_id);
            }
        }
    }

    void WifiConfigView::setup_ui()
    {
        // 1. Label: Redes Preferidas
        auto label = std::make_unique<Label>("Redes Preferidas");
        label->set_font_weight(FONT_WEIGHT_BOLD);
        label->set_fixed_size(24);
        m_title_label = label.get();
        add_child(std::move(label));

        // 2. TableView: SSID, Security
        auto table = std::make_unique<TableView<WifiNetwork>>();
        table->set_height(250);

        TableColumn<WifiNetwork> ssid_col;
        ssid_col.title = "Nombre de la red";
        ssid_col.width = 250;
        ssid_col.cell_factory = [](const WifiNetwork &data)
        { return std::make_unique<Label>(data.ssid); };
        table->add_column(std::move(ssid_col));

        TableColumn<WifiNetwork> security_col;
        security_col.title = "Tipo de seguridad";
        security_col.width = 150;
        security_col.cell_factory = [](const WifiNetwork &data)
        { return std::make_unique<Label>(data.security); };
        table->add_column(std::move(security_col));

        m_table_view = table.get();
        m_table_view->when_row_click.connect([this](auto &ctx)
                                             { this->on_network_selected(ctx.row_data); });
        add_child(std::move(table));

        // 3. Buttons: Agregar, Quitar (Horizontal Container)
        auto button_container = std::make_unique<Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(35);
        button_container->set_spacing(10);

        auto add_btn = std::make_unique<Button<SolidObject>>();
        add_btn->set_text("Agregar");

        m_add_button = add_btn.get();
        button_container->add_child(std::move(add_btn));

        auto remove_btn = std::make_unique<Button<SolidObject>>();
        remove_btn->set_text("Eliminar");

        m_remove_button = remove_btn.get();
        button_container->add_child(std::move(remove_btn));

        auto refresh_btn = std::make_unique<Button<SolidObject>>();
        refresh_btn->set_text("Refrescar");

        refresh_btn->when_click.connect([this](MouseButtonEventContext &)
                                        { this->refresh_networks(); });
        m_refresh_button = refresh_btn.get();
        button_container->add_child(std::move(refresh_btn));

        // Spacer to the right of buttons
        button_container->add_child(Spacer());

        add_child(std::move(button_container));

        // 4. Checkbox: Recordar las redes...
        auto checkbox = std::make_unique<Checkbox<AquaObject>>();
        checkbox->set_text("Recordar las redes a las que la computadora se ha unido");
        m_remember_checkbox = checkbox.get();
        add_child(std::move(checkbox));

        // Spacer below checkbox to fill vertical space
        // add_child(Spacer());
    }

    void WifiConfigView::refresh_networks()
    {
        if (m_table_view)
        {
            m_table_view->set_data(scan_networks());

            // Since RequestScan is async, we poll again in 3 seconds to show new results
            if (auto *app = application())
            {
                if (m_refresh_timer_id != 0)
                {
                    app->stop_timer(m_refresh_timer_id);
                }
                m_refresh_timer_id = app->add_timer(3000,
                                                    [this]()
                                                    {
                                                        m_refresh_timer_id = 0;
                                                        if (m_table_view)
                                                        {
                                                            m_table_view->set_data(scan_networks());
                                                        }
                                                    });
            }
        }
    }

    std::vector<WifiNetwork> WifiConfigView::scan_networks()
    {
        std::vector<WifiNetwork> networks;
        if (!m_dbus)
            return networks;

        // 1. Get Devices
        auto msg =
            m_dbus->call_method("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
                                "org.freedesktop.NetworkManager", "GetDevices");
        if (!msg)
            return networks;

        auto devices = m_dbus->get_object_path_list(msg);
        dbus_message_unref(msg);

        m_scan_devices.clear();
        std::vector<std::string> wifi_device_paths;
        for (const auto &path : devices)
        {
            auto type_var =
                m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                     "org.freedesktop.NetworkManager.Device", "DeviceType");
            if (std::holds_alternative<uint32_t>(type_var) &&
                std::get<uint32_t>(type_var) == 2) // NM_DEVICE_TYPE_WIFI = 2
            {
                // Get interface name (e.g. wlo1)
                auto iface_var =
                    m_dbus->get_property("org.freedesktop.NetworkManager", path,
                                         "org.freedesktop.NetworkManager.Device", "Interface");
                std::string iface_name = std::holds_alternative<std::string>(iface_var)
                                             ? std::get<std::string>(iface_var)
                                             : "wlan0";
                m_scan_devices.push_back({iface_name, path});
                wifi_device_paths.push_back(path);

                // Trigger scan
                m_dbus->call_void_method_with_empty_dict(
                    "org.freedesktop.NetworkManager", path,
                    "org.freedesktop.NetworkManager.Device.Wireless", "RequestScan");
            }
        }

        if (wifi_device_paths.empty())
            return networks;

        // 2. Get Access Points from all wifi devices
        for (const auto &wifi_device_path : wifi_device_paths)
        {
            msg = m_dbus->call_method("org.freedesktop.NetworkManager", wifi_device_path,
                                      "org.freedesktop.NetworkManager.Device.Wireless",
                                      "GetAllAccessPoints");
            if (!msg)
                continue;

            auto ap_paths = m_dbus->get_object_path_list(msg);
            dbus_message_unref(msg);

            for (const auto &ap_path : ap_paths)
            {
                auto ssid_var =
                    m_dbus->get_property("org.freedesktop.NetworkManager", ap_path,
                                         "org.freedesktop.NetworkManager.AccessPoint", "Ssid");
                auto wpa_var =
                    m_dbus->get_property("org.freedesktop.NetworkManager", ap_path,
                                         "org.freedesktop.NetworkManager.AccessPoint", "WpaFlags");
                auto rsn_var =
                    m_dbus->get_property("org.freedesktop.NetworkManager", ap_path,
                                         "org.freedesktop.NetworkManager.AccessPoint", "RsnFlags");

                std::string ssid_str = "";
                if (std::holds_alternative<std::vector<uint8_t>>(ssid_var))
                {
                    auto bytes = std::get<std::vector<uint8_t>>(ssid_var);
                    ssid_str = std::string(bytes.begin(), bytes.end());
                }

                if (ssid_str.empty())
                    ssid_str = "<Red Oculta>";

                uint32_t wpa =
                    std::holds_alternative<uint32_t>(wpa_var) ? std::get<uint32_t>(wpa_var) : 0;
                uint32_t rsn =
                    std::holds_alternative<uint32_t>(rsn_var) ? std::get<uint32_t>(rsn_var) : 0;

                networks.push_back({ssid_str, get_security_string(wpa, rsn), ap_path});
            }
        }

        return networks;
    }

    void WifiConfigView::on_network_selected(const WifiNetwork &network)
    {
        if (!m_initialized)
        {
            std::cerr << "WifiConfigView: Ignoring on_network_selected for SSID: " << network.ssid
                      << " (Initial load protection)" << std::endl;
            return;
        }

        if (m_dialog_open)
        {
            std::cerr << "WifiConfigView: Ignoring on_network_selected because another dialog is "
                         "already open."
                      << std::endl;
            return;
        }

        std::cerr << "WifiConfigView: on_network_selected called for SSID: " << network.ssid
                  << " at path: " << network.path << std::endl;

        if (m_scan_devices.empty())
            return;

        m_dialog_open = true;
        auto dialog =
            std::make_unique<WifiConnectDialog>(network.ssid, network.path, m_scan_devices);

        dialog->when_close.connect([this](EventContext &) { m_dialog_open = false; });

        std::thread(
            [d = std::move(dialog)]() mutable
            {
                d->initialize();
                d->run();
            })
            .detach();
    }
} // namespace horizon::preferences
