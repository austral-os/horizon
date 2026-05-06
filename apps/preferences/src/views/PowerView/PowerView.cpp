#include <filesystem>
#include <fstream>
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/UnderConstruction.hpp>
#include <iomanip>
#include <sstream>
#include <utils/ConfigUtils.hpp>
#include <views/PowerView/PowerView.hpp>

namespace horizon::preferences
{
    PowerView::PowerView() : Widget()
    {
        m_is_loading = true;
        m_config = std::make_unique<ConfigManager>(get_config_path("power.json"));
        m_config->load();

        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(0);
        set_spacing(0);

        auto notebook = std::make_unique<Notebook>();
        m_notebook = notebook.get();

        // AC Page
        m_notebook->add_tab(NotebookPage(i18n().tr("preferences.power.ac_power"),
                                         "ac-adapter-symbolic", create_power_page(false)));

        // Battery Page (Conditional)
        if (has_battery())
        {
            m_notebook->add_tab(NotebookPage(i18n().tr("preferences.power.battery_power"),
                                             "battery-symbolic", create_power_page(true)));
        }

        add_child(std::move(notebook));

        // Load configuration
        from_json(m_config->get_section("power"));
        m_is_loading = false;
    }

    std::unique_ptr<Widget> PowerView::create_power_page(bool is_battery)
    {
        auto page = std::make_unique<Widget>();
        page->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        page->set_spacing(30);
        page->set_margin(20);

        auto &controls = is_battery ? m_battery_controls : m_ac_controls;

        auto create_row = [](const std::string &label_text, std::unique_ptr<Widget> control,
                             Label **suffix_lbl, Checkbox<AquaObject> **check_ptr = nullptr)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_spacing(20);
            row->set_fixed_size(40);

            if (check_ptr)
            {
                auto cb = std::make_unique<Checkbox<AquaObject>>();
                cb->set_text(label_text);
                cb->set_fixed_size(300);
                *check_ptr = cb.get();
                row->add_child(std::move(cb));
            }
            else
            {
                auto label = std::make_unique<Label>(label_text);
                label->set_fixed_size(300);
                row->add_child(std::move(label));
            }

            control->set_position_type(WidgetPositionTypes::FILL);
            auto control_ptr = control.get();
            row->add_child(std::move(control));

            auto s_label = std::make_unique<Label>("");
            s_label->set_fixed_size(110);
            *suffix_lbl = s_label.get();
            row->add_child(std::move(s_label));

            return std::make_pair(std::move(row), control_ptr);
        };

        // --- Pantalla y brillo ---
        auto brightness_group = std::make_unique<Widget>();
        brightness_group->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        brightness_group->set_spacing(15);

        // Modo de energía
        auto profile_title = std::make_unique<Label>(i18n().tr("preferences.power.power_mode"));
        profile_title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        profile_title->set_font_size(18);
        profile_title->set_fixed_size(30);
        brightness_group->add_child(std::move(profile_title));

        auto c_profile = std::make_unique<Combo>();
        c_profile->add_item("power-saver", i18n().tr("preferences.power.power_saver"));
        c_profile->add_item("balanced", i18n().tr("preferences.power.balanced"));
        c_profile->add_item("performance", i18n().tr("preferences.power.performance"));
        c_profile->set_selected_item_index(1);
        Label *dummy_lbl_profile;
        auto [r_profile, cp_ptr] = create_row(i18n().tr("preferences.power.select_profile"),
                                              std::move(c_profile), &dummy_lbl_profile);
        controls.profile_combo = dynamic_cast<Combo *>(cp_ptr);
        controls.profile_combo->when_item_selected.connect(
            [this, is_battery](ComboItemSelectedContext &)
            {
                this->save_config();
                if (this->is_on_ac() == !is_battery)
                {
                    this->apply_power_profile();
                }
            });
        brightness_group->add_child(std::move(r_profile));

        // Espaciador entre secciones
        auto spacer = Spacer();
        spacer->set_fixed_size(20);
        brightness_group->add_child(std::move(spacer));

        auto section_title =
            std::make_unique<Label>(i18n().tr("preferences.power.display_and_brightness"));
        section_title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
        section_title->set_font_size(18);
        section_title->set_fixed_size(30);
        brightness_group->add_child(std::move(section_title));

        // Screen brightness
        auto s1 = std::make_unique<Slider>();
        auto [r1, s1_ptr] =
            create_row(i18n().tr("preferences.power.change_brightness"), std::move(s1),
                       &controls.brightness_label, &controls.brightness_check);
        controls.brightness_slider = dynamic_cast<Slider *>(s1_ptr);
        controls.brightness_slider->when_value_changed.connect(
            [this, is_battery, &controls](EventContext &)
            {
                int val = static_cast<int>(controls.brightness_slider->value() * 100);
                controls.brightness_label->set_text(std::to_string(val) + "%");

                // Aplicar solo si la pestaña coincide con el estado actual de energía
                if (this->is_on_ac() == !is_battery)
                {
                    this->apply_brightness(val);
                }
            });
        controls.brightness_slider->when_changed.connect([this](EventContext &)
                                                         { this->save_config(); });
        if (controls.brightness_check)
        {
            controls.brightness_check->when_toggle.connect([this](ToggleEventContext &)
                                                           { this->save_config(); });
        }
        brightness_group->add_child(std::move(r1));

        // Dim automatically
        auto c2 = std::make_unique<Combo>();
        c2->add_item("never", i18n().tr("preferences.power.never"));
        c2->add_item("1m", "1 " + i18n().tr("datetime.minutes.short.one"));
        c2->add_item("5m", "5 " + i18n().tr("datetime.minutes.short.other"));
        c2->add_item("10m", "10 " + i18n().tr("datetime.minutes.short.other"));
        c2->add_item("20m", "20 " + i18n().tr("datetime.minutes.short.other"));
        c2->add_item("30m", "30 " + i18n().tr("datetime.minutes.short.other"));
        c2->add_item("60m", "60 " + i18n().tr("datetime.minutes.short.other"));
        c2->set_selected_item_index(0);
        Label *dummy_lbl2;
        auto [r2, c2_ptr] = create_row(i18n().tr("preferences.power.dim_automatically"),
                                       std::move(c2), &dummy_lbl2);
        controls.dim_combo = dynamic_cast<Combo *>(c2_ptr);
        controls.dim_combo->when_item_selected.connect([this](ComboItemSelectedContext &)
                                                       { this->save_config(); });
        brightness_group->add_child(std::move(r2));

        // Turn off display
        auto c3 = std::make_unique<Combo>();
        c3->add_item("never", i18n().tr("preferences.power.never"));
        c3->add_item("1m", "1 " + i18n().tr("datetime.minutes.short.one"));
        c3->add_item("5m", "5 " + i18n().tr("datetime.minutes.short.other"));
        c3->add_item("10m", "10 " + i18n().tr("datetime.minutes.short.other"));
        c3->add_item("20m", "20 " + i18n().tr("datetime.minutes.short.other"));
        c3->add_item("30m", "30 " + i18n().tr("datetime.minutes.short.other"));
        c3->add_item("60m", "60 " + i18n().tr("datetime.minutes.short.other"));
        c3->set_selected_item_index(0);
        Label *dummy_lbl3;
        auto [r3, c3_ptr] =
            create_row(i18n().tr("preferences.power.turn_off_display"), std::move(c3), &dummy_lbl3);
        controls.turn_off_combo = dynamic_cast<Combo *>(c3_ptr);
        controls.turn_off_combo->when_item_selected.connect([this](ComboItemSelectedContext &)
                                                            { this->save_config(); });
        brightness_group->add_child(std::move(r3));

        // --- Battery Limits ---
        if (is_battery)
        {
            auto charge_title =
                std::make_unique<Label>(i18n().tr("preferences.power.charge_limits"));
            charge_title->set_font_weight(FontWeight::FONT_WEIGHT_BOLD);
            charge_title->set_font_size(18);
            charge_title->set_fixed_size(30);
            brightness_group->add_child(std::move(charge_title));

            auto s_charge = std::make_unique<Slider>();
            s_charge->set_enable_range(true);
            s_charge->set_min(0.0f);
            s_charge->set_max(100.0f);
            s_charge->set_value(20.0f);
            s_charge->set_second_value(80.0f);

            auto [r_charge, sc_ptr] =
                create_row(i18n().tr("preferences.power.battery_charge_range"), std::move(s_charge),
                           &controls.charge_label);
            controls.charge_slider = dynamic_cast<Slider *>(sc_ptr);
            controls.charge_slider->when_value_changed.connect(
                [&controls](EventContext &)
                {
                    int v1 = static_cast<int>(controls.charge_slider->value());
                    int v2 = static_cast<int>(controls.charge_slider->second_value());
                    controls.charge_label->set_text(std::to_string(v1) + "-" + std::to_string(v2) +
                                                    "%");
                });
            controls.charge_slider->when_changed.connect(
                [this](EventContext &)
                {
                    this->save_config();
                    this->apply_system_settings();
                });
            brightness_group->add_child(std::move(r_charge));
        }

        page->add_child(std::move(brightness_group));
        return page;
    }

    void PowerView::from_json(const nlohmann::json &j)
    {
        auto load_page = [](const nlohmann::json &sj, PageControls &c)
        {
            if (sj.is_null())
                return;
            if (c.brightness_slider && sj.contains("brightness"))
                c.brightness_slider->set_value(sj["brightness"].get<float>() / 100.0f);
            if (c.profile_combo && sj.contains("profile"))
                c.profile_combo->set_selected_item_by_id(sj["profile"].get<std::string>());
            if (c.brightness_check && sj.contains("change_brightness"))
                c.brightness_check->set_checked(sj["change_brightness"].get<bool>());
            if (c.dim_combo && sj.contains("dim_after"))
                c.dim_combo->set_selected_item_by_id(sj["dim_after"].get<std::string>());
            if (c.turn_off_combo && sj.contains("turn_off_after"))
                c.turn_off_combo->set_selected_item_by_id(sj["turn_off_after"].get<std::string>());
            if (c.charge_slider && sj.contains("charge_limit_min") &&
                sj.contains("charge_limit_max"))
            {
                c.charge_slider->set_value(sj["charge_limit_min"].get<float>());
                c.charge_slider->set_second_value(sj["charge_limit_max"].get<float>());
            }
        };

        if (j.contains("ac"))
            load_page(j["ac"], m_ac_controls);
        if (j.contains("battery"))
            load_page(j["battery"], m_battery_controls);
    }

    nlohmann::json PowerView::to_json() const
    {
        auto save_page = [](const PageControls &c)
        {
            nlohmann::json sj;
            if (c.brightness_slider)
                sj["brightness"] = static_cast<int>(c.brightness_slider->value() * 100);
            if (c.profile_combo && c.profile_combo->selected_item())
                sj["profile"] = c.profile_combo->selected_item()->id;
            if (c.brightness_check)
                sj["change_brightness"] = c.brightness_check->is_checked();
            if (c.dim_combo && c.dim_combo->selected_item())
                sj["dim_after"] = c.dim_combo->selected_item()->id;
            if (c.turn_off_combo && c.turn_off_combo->selected_item())
                sj["turn_off_after"] = c.turn_off_combo->selected_item()->id;
            if (c.charge_slider)
            {
                sj["charge_limit_min"] = static_cast<int>(c.charge_slider->value());
                sj["charge_limit_max"] = static_cast<int>(c.charge_slider->second_value());
            }
            return sj;
        };

        nlohmann::json j;
        j["ac"] = save_page(m_ac_controls);
        if (has_battery())
        {
            j["battery"] = save_page(m_battery_controls);
        }
        return j;
    }

    void PowerView::save_config()
    {
        if (m_is_loading)
            return;
        m_config->set_section("power", to_json());
        m_config->save();
    }

    void PowerView::apply_system_settings()
    {
        if (m_is_loading)
            return;
        if (!has_battery() || !m_battery_controls.charge_slider)
            return;

        int start = static_cast<int>(m_battery_controls.charge_slider->value());
        int end = static_cast<int>(m_battery_controls.charge_slider->second_value());

        // Comando para aplicar límites de carga en dispositivos BAT*
        // Cubre tanto el estándar moderno como el de ThinkPads
        std::string cmd = "pkexec bash -c 'for bat in /sys/class/power_supply/BAT*; do "
                          "if [ -f \"$bat/charge_control_start_threshold\" ]; then "
                          "echo " +
                          std::to_string(start) +
                          " > \"$bat/charge_control_start_threshold\" && "
                          "echo " +
                          std::to_string(end) +
                          " > \"$bat/charge_control_end_threshold\"; "
                          "elif [ -f \"$bat/charge_start_threshold\" ]; then "
                          "echo " +
                          std::to_string(start) +
                          " > \"$bat/charge_start_threshold\" && "
                          "echo " +
                          std::to_string(end) +
                          " > \"$bat/charge_stop_threshold\"; "
                          "fi; done'";

        // Ejecutar de forma asíncrona para no bloquear la UI del Panel de Preferencias
        system((cmd + " &").c_str());
    }

    void PowerView::apply_brightness(int value)
    {
        if (m_is_loading)
            return;
        std::string cmd = "brightnessctl set " + std::to_string(value) + "%";
        system(cmd.c_str());
    }

    void PowerView::apply_power_profile()
    {
        if (m_is_loading)
            return;

        bool on_ac = is_on_ac();
        auto &controls = on_ac ? m_ac_controls : m_battery_controls;
        if (!controls.profile_combo || !controls.profile_combo->selected_item())
            return;

        std::string profile = controls.profile_combo->selected_item()->id;
        std::string cmd = "powerprofilesctl set " + profile;
        system((cmd + " &").c_str());
    }

    bool PowerView::is_on_ac() const
    {
        try
        {
            if (std::filesystem::exists("/sys/class/power_supply"))
            {
                for (const auto &entry :
                     std::filesystem::directory_iterator("/sys/class/power_supply"))
                {
                    std::string name = entry.path().filename().string();
                    if (name.find("AC") == 0 || name.find("ADP") == 0)
                    {
                        std::ifstream f(entry.path() / "online");
                        int online = 0;
                        if (f >> online)
                            return online == 1;
                    }
                }
            }
        }
        catch (...)
        {
        }
        return true; // Asumimos AC por defecto si no podemos determinarlo
    }

    bool PowerView::has_battery() const
    {
        try
        {
            if (std::filesystem::exists("/sys/class/power_supply"))
            {
                for (const auto &entry :
                     std::filesystem::directory_iterator("/sys/class/power_supply"))
                {
                    if (entry.path().filename().string().find("BAT") == 0)
                    {
                        return true;
                    }
                }
            }
        }
        catch (...)
        {
        }
        return false;
    }
} // namespace horizon::preferences
