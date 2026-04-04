#include <views/WifiView/WifiConfigView.hpp>
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Spacer.hpp>

namespace horizon::preferences
{
    WifiConfigView::WifiConfigView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_spacing(10);
        set_margin(10);

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
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("nmcli -t -f SSID,SECURITY device wifi list", "r"), pclose);

        if (!pipe)
        {
            return networks;
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result = buffer.data();
            // Remove newline if present
            if (!result.empty() && result.back() == '\n') result.pop_back();

            std::stringstream ss(result);
            std::string ssid, security;
            if (std::getline(ss, ssid, ':') && std::getline(ss, security))
            {
                if (!ssid.empty()) {
                    networks.push_back({ssid, security});
                }
            }
        }

        return networks;
    }
}
