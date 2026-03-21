#include <views/DisplayView/DisplayView.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/Application.hpp>

namespace horizon::preferences
{
    DisplayView::DisplayView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(FILL);
        set_margin(20);
        set_spacing(20);

        auto title = std::make_unique<Label>("Pantalla");
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));

        // 1. Display Devices (Upper Part)
        auto devices = std::make_unique<DisplayDevices>();
        devices->set_fixed_size(300); // Fixed height for the monitor visualization
        m_display_devices = devices.get();
        m_display_devices->when_monitor_selected.connect([this](int idx) { on_monitor_selected(idx); });
        add_child(std::move(devices));

        // 2. Controls Section (Lower Part)
        auto controls_container = std::make_unique<Widget>();
        controls_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        controls_container->set_position_type(FILL);
        controls_container->set_spacing(20);

        // 2.1 Resolution Table (Left)
        auto res_section = std::make_unique<Widget>();
        res_section->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        res_section->set_position_type(FILL);
        res_section->set_spacing(10);
        
        res_section->add_child(std::make_unique<Label>("Resolución"));

        auto table = std::make_unique<TableView<MonitorMode>>();
        m_res_table = table.get();
        
        TableColumn<MonitorMode> col_res;
        col_res.title = "Resolución";
        col_res.width = 150;
        col_res.cell_factory = [](const MonitorMode &m) {
            return std::make_unique<Label>(std::to_string(m.width) + " x " + std::to_string(m.height));
        };
        m_res_table->add_column(col_res);

        TableColumn<MonitorMode> col_hz;
        col_hz.title = "Refresco";
        col_hz.width = 80;
        col_hz.cell_factory = [](const MonitorMode &m) {
            return std::make_unique<Label>(std::to_string((int)m.refresh_rate) + " Hz");
        };
        m_res_table->add_column(col_hz);
        m_res_table->set_header_visible(false);

        res_section->add_child(std::move(table));
        controls_container->add_child(std::move(res_section));

        // 2.2 Other Settings (Right)
        auto settings_section = std::make_unique<Widget>();
        settings_section->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        settings_section->set_fixed_size(250); // Fixed width for combos
        settings_section->set_spacing(15);

        settings_section->add_child(std::make_unique<Label>("Rotación"));
        auto rot_combo = std::make_unique<Combo>();
        rot_combo->add_item("0", "Normal");
        rot_combo->add_item("90", "90° Derecha");
        rot_combo->add_item("180", "180° (Invertido)");
        rot_combo->add_item("270", "90° Izquierda");
        m_rotation_combo = rot_combo.get();
        settings_section->add_child(std::move(rot_combo));

        settings_section->add_child(std::make_unique<Label>("Velocidad de Refresco"));
        auto ref_combo = std::make_unique<Combo>();
        m_refresh_combo = ref_combo.get();
        settings_section->add_child(std::move(ref_combo));

        controls_container->add_child(std::move(settings_section));
        add_child(std::move(controls_container));
    }

    void DisplayView::on_monitor_selected(int index)
    {
        auto* surface = application()->w_surface();
        if (!surface || index < 0 || (size_t)index >= surface->monitor_details().size()) return;
        
        m_selected_monitor_idx = index;
        const auto &d = surface->monitor_details()[index];

        // Update resolution table
        std::vector<MonitorMode> modes;
        int current_idx = -1;
        for (const auto &m : d.modes) {
            MonitorMode mode;
            mode.width = m.width;
            mode.height = m.height;
            mode.refresh_rate = (float)m.refresh / 1000.0f;
            modes.push_back(mode);
            if (m.current) current_idx = (int)modes.size() - 1;
        }

        m_res_table->set_data(modes);
        if (current_idx != -1) {
            m_res_table->set_selected_index(current_idx);
        }

        // Update rotation (default to 0 for now as WaylandSurface doesn't capture it yet)
        m_rotation_combo->set_selected_item_by_id("0");

        // Update refresh rates combo
        m_refresh_combo->clear_items();
        std::set<int> rates;
        int current_w = 0, current_h = 0;
        if (current_idx != -1) {
            current_w = modes[current_idx].width;
            current_h = modes[current_idx].height;
        }

        for (const auto &mode : modes) {
            if (mode.width == current_w && mode.height == current_h) {
                rates.insert((int)mode.refresh_rate);
            }
        }

        for (int r : rates) {
            std::string rs = std::to_string(r);
            m_refresh_combo->add_item(rs, rs + " Hz");
        }
        
        if (current_idx != -1) {
            m_refresh_combo->set_selected_item_by_id(std::to_string((int)modes[current_idx].refresh_rate));
        }
    }
}
