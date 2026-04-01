#include <algorithm>
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Logger.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/WaylandWindow.hpp>
#include <views/DisplayView/DisplayView.hpp>
#include <views/DisplayView/KwinAdapter.hpp>
#include <views/DisplayView/LabwcAdapter.hpp>
#include <views/DisplayView/WayfireAdapter.hpp>

namespace horizon::preferences
{
    DisplayView::DisplayView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(FILL);
        set_margin(20);
        set_spacing(20);

        // Detect compositor and initialize adapter
        const char *desktop = std::getenv("XDG_CURRENT_DESKTOP");
        std::string desktop_str = desktop ? desktop : "";
        LOG_INFO << "[VIEW] Detected desktop: " << desktop_str;

        if (desktop_str.find("KDE") != std::string::npos)
        {
            m_adapter = std::make_unique<KwinAdapter>();
        }
        else if (desktop_str == "Wayfire")
        {
            m_adapter = std::make_unique<WayfireAdapter>();
        }
        else
        {
            m_adapter = std::make_unique<LabwcAdapter>();
        }

        auto title = std::make_unique<Label>("Pantalla");
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));

        // Wait for monitor details to be populated safely
        when_application_load.connect(
            [this](EventContext &)
            {
                if (!m_monitors.empty())
                    return;
                auto *app = application();
                if (app && app->w_surface())
                {
                    for (const auto &d : app->w_surface()->monitor_details())
                    {
                        MonitorInfo info;
                        info.conn_name = d.name;
                        info.x = d.x;
                        info.y = d.y;
                        info.width = d.width;
                        info.height = d.height;
                        info.rotation = 0;
                        m_monitors.push_back(info);
                        LOG_INFO << "[VIEW] Added monitor: " << info.conn_name << " at " << info.x
                                 << "," << info.y;
                    }
                }
            });

        // 1. Display Devices (Upper Part)
        auto devices = std::make_unique<DisplayDevices>();
        // devices->set_fixed_size(300); // Fixed height for the monitor visualization
        m_display_devices = devices.get();
        m_display_devices->when_monitor_selected.connect([this](int idx)
                                                         { on_monitor_selected(idx); });
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

        auto lbl_restitle = std::make_unique<Label>("Resolución");
        lbl_restitle->set_fixed_size(32);

        res_section->add_child(std::move(lbl_restitle));

        auto native_check = std::make_unique<Checkbox<AquaObject>>();
        m_native_res_checkbox = native_check.get();
        m_native_res_checkbox->set_fixed_size(25);
        m_native_res_checkbox->set_text("Usar resolución nativa");
        m_native_res_checkbox->set_checked(true);
        m_native_res_checkbox->set_on_toggle(
            [this](bool checked)
            {
                m_res_table->set_enabled(!checked);
                on_monitor_selected(m_selected_monitor_idx);
            });
        res_section->add_child(std::move(native_check));

        auto table = std::make_unique<TableView<MonitorMode>>();
        m_res_table = table.get();

        TableColumn<MonitorMode> col_res;
        col_res.title = "Resolución";
        col_res.width = 150;
        col_res.cell_factory = [](const MonitorMode &m)
        {
            return std::make_unique<Label>(std::to_string(m.width) + " x " +
                                           std::to_string(m.height));
        };
        m_res_table->add_column(col_res);

        TableColumn<MonitorMode> col_hz;
        col_hz.title = "Refresco";
        col_hz.width = 80;
        col_hz.cell_factory = [](const MonitorMode &m)
        { return std::make_unique<Label>(std::to_string((int)m.refresh_rate) + " Hz"); };
        m_res_table->add_column(col_hz);
        m_res_table->set_header_visible(false);
        m_res_table->set_enabled(false); // Initial state (checked=true)

        res_section->add_child(std::move(table));
        controls_container->add_child(std::move(res_section));

        // 2.2 Other Settings (Right)
        auto settings_section = std::make_unique<Widget>();
        settings_section->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        settings_section->set_fixed_size(300); // Fixed width for combos
        settings_section->set_spacing(8);

        auto lbl_rot = std::make_unique<Label>("Rotación");
        lbl_rot->set_fixed_size(32);

        settings_section->add_child(std::move(lbl_rot));
        auto rot_combo = std::make_unique<Combo>();
        rot_combo->set_fixed_size(32);
        rot_combo->add_item("0", "Normal");
        rot_combo->add_item("90", "90° Derecha");
        rot_combo->add_item("180", "180° (Invertido)");
        rot_combo->add_item("270", "90° Izquierda");
        m_rotation_combo = rot_combo.get();
        settings_section->add_child(std::move(rot_combo));

        auto lbl_speed = std::make_unique<Label>("Velocidad de Refresco");
        lbl_speed->set_fixed_size(32);

        settings_section->add_child(std::move(lbl_speed));
        auto ref_combo = std::make_unique<Combo>();
        ref_combo->set_fixed_size(32);
        m_refresh_combo = ref_combo.get();
        settings_section->add_child(std::move(ref_combo));

        // 2.2.3 Apply Button
        auto apply_btn = std::make_unique<Button<AquaObject>>();
        m_apply_button = apply_btn.get();
        m_apply_button->set_text("Aplicar Cambios");
        m_apply_button->set_fixed_size(36);
        m_apply_button->when_mouse_press.connect(
            [this](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110 && m_adapter)
                {
                    std::vector<MonitorConfig> current_configs;
                    std::vector<MonitorConfig> configs;
                    for (size_t i = 0; i < m_monitors.size(); ++i)
                    {
                        const auto &m = m_monitors[i];
                        MonitorConfig cfg;
                        cfg.name = m.conn_name;
                        cfg.x = m.x;
                        cfg.y = m.y;
                        cfg.width = m.width;
                        cfg.height = m.height;
                        cfg.refresh = 60.0f;
                        cfg.enabled = true;
                        cfg.rotation = m.rotation;

                        current_configs.push_back(cfg); // Save current as old before change

                        // If it's the selected monitor, use current selections from UI
                        if ((int)i == m_selected_monitor_idx)
                        {
                            auto selected_mode_idx = m_res_table->selected_index();
                            if (selected_mode_idx != -1)
                            {
                                auto mode = m_res_table->data()[selected_mode_idx];
                                cfg.width = mode.width;
                                cfg.height = mode.height;
                                cfg.refresh = mode.refresh_rate;
                            }
                            // Also get rotation
                            auto *rot_item = m_rotation_combo->selected_item();
                            if (rot_item)
                            {
                                cfg.rotation = std::stoi(rot_item->id);
                            }
                        }
                        configs.push_back(cfg);
                    }

                    LOG_INFO << "DisplayView: Applying " << configs.size() << " configs";
                    for (const auto &c : configs)
                    {
                        LOG_INFO << "  Monitor: " << c.name << " Res: " << c.width << "x"
                                 << c.height << "@" << (int)c.refresh << " Rot: " << c.rotation;
                    }

                    m_adapter->apply_configs(configs);

                    // Show confirmation
                    m_previous_configs = current_configs; // Capture old
                    show_confirmation();
                }
            });
        settings_section->add_child(horizon::Spacer());
        settings_section->add_child(std::move(apply_btn));

        controls_container->add_child(std::move(settings_section));
        add_child(std::move(controls_container));

        // Create hidden overlay
        auto overlay = std::make_unique<SolidObject>();
        m_overlay = overlay.get();
        m_overlay->set_background_color(Color(0.0f, 0.0f, 0.0f, 0.6f));
        m_overlay->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        m_overlay->set_position_type(FREE); // Overlap
        m_overlay->set_visible(false);

        m_overlay->add_child(Spacer()); // Vertical center - top spacer

        auto h_container = std::make_unique<Widget>();
        h_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        h_container->add_child(Spacer()); // Horizontal center - left spacer

        auto box = std::make_unique<AquaObject>();
        box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        box->set_margin(30);
        box->set_spacing(20);
        box->set_fixed_size(450); // Square-ish
        box->set_corner_radius({12, 12, 12, 12});

        auto msg = std::make_unique<Label>("¿Desea mantener esta configuración de pantalla?");
        msg->set_font_size(18);
        msg->set_alignment(TextAlignment::Center);
        box->add_child(std::move(msg));

        auto countdown = std::make_unique<Label>("Revirtiendo en 10 segundos...");
        m_countdown_label = countdown.get();
        m_countdown_label->set_alignment(TextAlignment::Center);
        box->add_child(std::move(countdown));

        auto btns = std::make_unique<Widget>();
        btns->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        btns->set_spacing(20);

        btns->add_child(Spacer()); // Buttons center - left

        auto keep = std::make_unique<Button<AquaObject>>();
        keep->set_text("Mantener");
        keep->set_size(140, 40);
        keep->when_mouse_press.connect(
            [this](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110)
                {
                    if (m_confirmation_timer_id != 0)
                    {
                        application()->stop_timer(m_confirmation_timer_id);
                        m_confirmation_timer_id = 0;
                    }
                    m_overlay->set_visible(false);
                    invalidate();
                }
            });
        btns->add_child(std::move(keep));

        auto revert_btn = std::make_unique<Button<SolidObject>>();
        revert_btn->set_text("Revertir");
        revert_btn->set_size(140, 40);
        revert_btn->when_mouse_press.connect(
            [this](MouseButtonEventContext &ctx)
            {
                if (ctx.button == 0x110)
                {
                    revert_settings();
                }
            });
        btns->add_child(std::move(revert_btn));

        btns->add_child(Spacer()); // Buttons center - right

        box->add_child(std::move(btns));
        h_container->add_child(std::move(box));
        h_container->add_child(Spacer()); // Horizontal center - right spacer

        m_overlay->add_child(std::move(h_container));
        m_overlay->add_child(Spacer()); // Vertical center - bottom spacer

        add_child(std::move(overlay));
    }

    void DisplayView::show_confirmation()
    {
        m_countdown = 10;
        m_countdown_label->set_text("Revirtiendo en " + std::to_string(m_countdown) +
                                    " segundos...");
        m_overlay->set_visible(true);
        m_overlay->set_size(this->width(), this->height());

        if (m_confirmation_timer_id != 0)
        {
            application()->stop_timer(m_confirmation_timer_id);
        }

        m_confirmation_timer_id = application()->add_timer(
            1000,
            [this]()
            {
                m_countdown--;
                if (m_countdown <= 0)
                {
                    revert_settings();
                }
                else
                {
                    m_countdown_label->set_text("Revirtiendo en " + std::to_string(m_countdown) +
                                                " segundos...");
                    invalidate();
                }
            },
            true);

        invalidate();
    }

    void DisplayView::revert_settings()
    {
        if (m_confirmation_timer_id != 0)
        {
            application()->stop_timer(m_confirmation_timer_id);
            m_confirmation_timer_id = 0;
        }

        if (m_adapter && !m_previous_configs.empty())
        {
            m_adapter->apply_configs(m_previous_configs);
        }

        m_overlay->set_visible(false);
        on_monitor_selected(m_selected_monitor_idx); // Refresh UI to match reverted state
        invalidate();
    }

    void DisplayView::on_monitor_selected(int index)
    {
        if (index < 0 || index >= (int)m_display_devices->monitors().size())
            return;

        m_selected_monitor_idx = index;
        const auto &d = m_display_devices->monitors()[index].info;

        // Update resolution table
        std::vector<MonitorMode> modes = d.modes;
        int current_idx = d.current_mode_index;
        
        if (!modes.empty())
        {
            // Sort by resolution area descending
            std::sort(modes.begin(), modes.end(),
                      [](const MonitorMode &a, const MonitorMode &b)
                      {
                          if (a.width * a.height != b.width * b.height)
                              return (a.width * a.height) > (b.width * b.height);
                          return a.refresh_rate > b.refresh_rate;
                      });

            // Re-find current index after sort
            if (m_native_res_checkbox->is_checked())
            {
                // Select native resolution (max area)
                int max_area = 0;
                int best_idx = 0;
                for (size_t i = 0; i < modes.size(); ++i)
                {
                    int area = modes[i].width * modes[i].height;
                    if (area > max_area)
                    {
                        max_area = area;
                        best_idx = (int)i;
                    }
                }
                current_idx = best_idx;
            }
            else if (current_idx != -1)
            {
                // Find by values since indices changed
                int cur_w = d.modes[current_idx].width;
                int cur_h = d.modes[current_idx].height;
                for (size_t i = 0; i < modes.size(); ++i)
                {
                    if (modes[i].width == cur_w && modes[i].height == cur_h)
                    {
                        current_idx = (int)i;
                        break;
                    }
                }
            }
        }

        m_res_table->set_data(modes);
        if (current_idx != -1)
        {
            m_res_table->set_selected_index(current_idx);
        }

        // Update rotation (default to 0 for now as WaylandSurface doesn't capture it yet)
        m_rotation_combo->set_selected_item_by_id("0");

        // Update refresh rates combo
        m_refresh_combo->clear_items();
        std::set<int> rates;
        int current_w = 0, current_h = 0;
        if (current_idx != -1)
        {
            current_w = modes[current_idx].width;
            current_h = modes[current_idx].height;
        }

        for (const auto &mode : modes)
        {
            if (mode.width == current_w && mode.height == current_h)
            {
                rates.insert((int)mode.refresh_rate);
            }
        }

        for (int r : rates)
        {
            std::string rs = std::to_string(r);
            m_refresh_combo->add_item(rs, rs + " Hz");
        }

        if (current_idx != -1)
        {
            m_refresh_combo->set_selected_item_by_id(
                std::to_string((int)modes[current_idx].refresh_rate));
        }
    }
} // namespace horizon::preferences
