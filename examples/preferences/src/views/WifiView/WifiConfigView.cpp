#include <views/WifiView/WifiConfigView.hpp>
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Spacer.hpp>
#include <iostream>

namespace horizon::preferences
{
    static std::string get_security_string(uint32_t wpa, uint32_t rsn)
    {
        std::string security = "";
        if (rsn != 0) security += "WPA2 ";
        if (wpa != 0) security += "WPA ";
        if (security.empty()) security = "None";
        return security;
    }

    WifiConfigView::WifiConfigView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(10);
        set_margin(10);

        try {
            m_dbus = std::make_unique<dbusutils::DbusHelper>(DBUS_BUS_SYSTEM);
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize D-Bus: " << e.what() << std::endl;
        }

        setup_ui();
        refresh_networks();
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
        ssid_col.cell_factory = [](const WifiNetwork& data) {
            return std::make_unique<Label>(data.ssid);
        };
        table->add_column(std::move(ssid_col));

        TableColumn<WifiNetwork> security_col;
        security_col.title = "Tipo de seguridad";
        security_col.width = 150;
        security_col.cell_factory = [](const WifiNetwork& data) {
            return std::make_unique<Label>(data.security);
        };
        table->add_column(std::move(security_col));

        m_table_view = table.get();
        add_child(std::move(table));

        // 3. Buttons: Agregar, Quitar (Horizontal Container)
        auto button_container = std::make_unique<Widget>();
        button_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        button_container->set_fixed_size(30); 
        button_container->set_spacing(10);

        auto add_btn = std::make_unique<Button<AquaObject>>();
        add_btn->set_text("Agregar");
        add_btn->set_size(100, 32);
        m_add_button = add_btn.get();
        button_container->add_child(std::move(add_btn));

        auto remove_btn = std::make_unique<Button<AquaObject>>();
        remove_btn->set_text("Quitar");
        remove_btn->set_size(100, 32);
        m_remove_button = remove_btn.get();
        button_container->add_child(std::move(remove_btn));

        // Spacer to the right of buttons
        button_container->add_child(Spacer());

        add_child(std::move(button_container));

        // 4. Checkbox: Recordar las redes...
        auto checkbox = std::make_unique<Checkbox<AquaObject>>();
        checkbox->set_text("Recordar las redes a las que la computadora se ha unido");
        m_remember_checkbox = checkbox.get();
        add_child(std::move(checkbox));

        // Spacer below checkbox to fill vertical space
        add_child(Spacer());
    }

    void WifiConfigView::refresh_networks()
    {
        if (m_table_view)
        {
            m_table_view->set_data(scan_networks());
        }
    }

    std::vector<WifiNetwork> WifiConfigView::scan_networks()
    {
        std::vector<WifiNetwork> networks;
        if (!m_dbus) return networks;

        // 1. Get Devices
        auto msg = m_dbus->call_method("org.freedesktop.NetworkManager", 
                                       "/org/freedesktop/NetworkManager", 
                                       "org.freedesktop.NetworkManager", 
                                       "GetDevices");
        if (!msg) return networks;

        auto devices = m_dbus->get_object_path_list(msg);
        dbus_message_unref(msg);

        std::string wifi_device_path = "";
        for (const auto& path : devices)
        {
            auto type_var = m_dbus->get_property("org.freedesktop.NetworkManager", path, "org.freedesktop.NetworkManager.Device", "DeviceType");
            if (std::holds_alternative<uint32_t>(type_var) && std::get<uint32_t>(type_var) == 2) // NM_DEVICE_TYPE_WIFI = 2
            {
                wifi_device_path = path;
                break;
            }
        }

        if (wifi_device_path.empty()) return networks;

        // 2. Get Access Points
        msg = m_dbus->call_method("org.freedesktop.NetworkManager", wifi_device_path, "org.freedesktop.NetworkManager.Device.Wireless", "GetAllAccessPoints");
        if (!msg) return networks;

        auto ap_paths = m_dbus->get_object_path_list(msg);
        dbus_message_unref(msg);

        for (const auto& ap_path : ap_paths)
        {
            auto ssid_var = m_dbus->get_property("org.freedesktop.NetworkManager", ap_path, "org.freedesktop.NetworkManager.AccessPoint", "Ssid");
            auto wpa_var = m_dbus->get_property("org.freedesktop.NetworkManager", ap_path, "org.freedesktop.NetworkManager.AccessPoint", "WpaFlags");
            auto rsn_var = m_dbus->get_property("org.freedesktop.NetworkManager", ap_path, "org.freedesktop.NetworkManager.AccessPoint", "RsnFlags");

            std::string ssid_str = "";
            if (std::holds_alternative<std::vector<uint8_t>>(ssid_var))
            {
                auto bytes = std::get<std::vector<uint8_t>>(ssid_var);
                ssid_str = std::string(bytes.begin(), bytes.end());
            }

            uint32_t wpa = std::holds_alternative<uint32_t>(wpa_var) ? std::get<uint32_t>(wpa_var) : 0;
            uint32_t rsn = std::holds_alternative<uint32_t>(rsn_var) ? std::get<uint32_t>(rsn_var) : 0;

            if (!ssid_str.empty())
            {
                networks.push_back({ssid_str, get_security_string(wpa, rsn)});
            }
        }

        return networks;
    }
}
