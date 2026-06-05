#include "horizon/Widget.hpp"
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <horizon/Frame.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <utils/ConfigUtils.hpp>
#include <views/AppearanceView/AppearanceView.hpp>

namespace horizon::preferences
{
    AppearanceView::AppearanceView() : Widget()
    {
        m_is_loading = true;
        m_config = std::make_unique<ConfigManager>(get_config_path("color-scheme.json"));
        m_config->load();

        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(20);

        from_json(m_config->get_section("variant"));

        auto panel_sec = m_config->get_section("panel_opacity");
        if (panel_sec.is_number())
        {
            m_panel_opacity = panel_sec.get<float>();
        }
        else
        {
            m_panel_opacity = 1.0f;
        }

        auto menu_sec = m_config->get_section("menu_opacity");
        if (menu_sec.is_number())
        {
            m_menu_opacity = menu_sec.get<float>();
        }
        else
        {
            m_menu_opacity = 1.0f;
        }

        load_compositor_config();
        setup_ui();
        m_is_loading = false;
    }

    void AppearanceView::setup_ui()
    {
        clear_children();

        const int LEFT_LABELS_WIDTH = 300;

        // Title
        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.appearance"));
        title->set_font_size(18);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_fixed_size(40);
        m_title_label = title.get();
        add_child(std::move(title));

        // Appearance Section Row
        auto appearance_row = std::make_unique<Widget>();
        appearance_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        appearance_row->set_spacing(20);
        appearance_row->set_fixed_size(120);

        auto appearance_label =
            std::make_unique<Label>(i18n().tr("preferences.appearance.title") + ":");
        appearance_label->set_fixed_size(LEFT_LABELS_WIDTH);
        appearance_label->set_alignment(TextAlignment::Right);
        appearance_label->set_vertical_alignment(VerticalAlignment::Top);
        appearance_row->add_child(std::move(appearance_label));

        // Light Theme Box
        auto light_box = std::make_unique<Widget>();
        light_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        light_box->set_spacing(10);
        light_box->set_margin(10);
        light_box->set_fixed_size(120);
        light_box->set_border_radius(8);
        light_box->set_cursor_type(CursorType::Pointer);

        auto light_icon = std::make_unique<Icon>();
        light_icon->set_icon_name("theme-variant-light");
        light_icon->set_icon_size(80);
        light_icon->set_position_type(WidgetPositionTypes::FILL);
        light_box->add_child(std::move(light_icon));

        auto light_label = std::make_unique<Label>(i18n().tr("preferences.appearance.light"));
        light_label->set_alignment(TextAlignment::Center);
        light_label->set_fixed_size(20);
        light_box->add_child(std::move(light_label));

        light_box->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                m_variant = "light";
                save_config();
                update_selection_visuals();
            });
        m_light_box = light_box.get();
        appearance_row->add_child(std::move(light_box));

        // Dark Theme Box
        auto dark_box = std::make_unique<Widget>();
        dark_box->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        dark_box->set_spacing(10);
        dark_box->set_margin(10);
        dark_box->set_fixed_size(120);
        dark_box->set_border_radius(8);
        dark_box->set_cursor_type(CursorType::Pointer);

        auto dark_icon = std::make_unique<Icon>();
        dark_icon->set_icon_name("theme-variant-dark");
        dark_icon->set_icon_size(80);
        dark_icon->set_position_type(WidgetPositionTypes::FILL);
        dark_box->add_child(std::move(dark_icon));

        auto dark_label = std::make_unique<Label>(i18n().tr("preferences.appearance.dark"));
        dark_label->set_alignment(TextAlignment::Center);
        dark_label->set_fixed_size(20);
        dark_box->add_child(std::move(dark_label));

        dark_box->when_click.connect(
            [this](MouseButtonEventContext &)
            {
                m_variant = "dark";
                save_config();
                update_selection_visuals();
            });
        m_dark_box = dark_box.get();
        appearance_row->add_child(std::move(dark_box));

        appearance_row->add_child(Spacer());

        add_child(std::move(appearance_row));

        // Compositor Section Row
        auto compositor_row = std::make_unique<Widget>();
        compositor_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        compositor_row->set_spacing(20);
        compositor_row->set_fixed_size(40);

        auto compositor_label =
            std::make_unique<Label>(i18n().tr("preferences.appearance.compositor") + ":");
        compositor_label->set_fixed_size(LEFT_LABELS_WIDTH);
        compositor_label->set_alignment(TextAlignment::Right);
        compositor_label->set_vertical_alignment(VerticalAlignment::Middle);
        compositor_row->add_child(std::move(compositor_label));

        auto checkbox = std::make_unique<Checkbox<AquaObject>>();
        checkbox->set_text(i18n().tr("preferences.appearance.use_graphic_effects"));
        checkbox->set_fixed_size(300);
        checkbox->set_checked(m_use_effects);
        checkbox->when_toggle.connect([this](ToggleEventContext &ctx)
                                      { set_use_effects(ctx.checked); });
        m_compositor_checkbox = checkbox.get();
        compositor_row->add_child(std::move(checkbox));

        compositor_row->add_child(Spacer());
        add_child(std::move(compositor_row));

        // Slider rows helper
        auto create_slider_row = [this](const std::string &label_text, float init_val,
                                        Slider **slider_out, Label **label_out)
        {
            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_spacing(20);
            row->set_fixed_size(40);

            auto lbl = std::make_unique<Label>(label_text);
            lbl->set_fixed_size(LEFT_LABELS_WIDTH);
            lbl->set_alignment(TextAlignment::Right);
            lbl->set_vertical_alignment(VerticalAlignment::Middle);
            row->add_child(std::move(lbl));

            auto sl_container = std::make_unique<Widget>();
            sl_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            sl_container->set_fixed_size(300);
            sl_container->set_spacing(10);

            auto sl = std::make_unique<Slider>();
            sl->set_min(50.0f);
            sl->set_max(100.0f);
            sl->set_value(init_val * 100.0f);
            sl->set_fixed_size(230);
            *slider_out = sl.get();

            auto val_lbl =
                std::make_unique<Label>(std::to_string(static_cast<int>(init_val * 100.0f)) + "%");
            val_lbl->set_fixed_size(60);
            val_lbl->set_alignment(TextAlignment::Left);
            val_lbl->set_vertical_alignment(VerticalAlignment::Middle);
            *label_out = val_lbl.get();

            sl->when_value_changed.connect(
                [val_lbl_ptr = val_lbl.get(), sl_ptr = sl.get()](EventContext &)
                {
                    float val = sl_ptr->value();
                    val_lbl_ptr->set_text(std::to_string(static_cast<int>(val)) + "%");
                });

            sl->when_changed.connect([this](EventContext &) { this->save_config(); });

            sl_container->add_child(std::move(sl));
            sl_container->add_child(std::move(val_lbl));

            row->add_child(std::move(sl_container));
            row->add_child(Spacer());

            return row;
        };

        // Panel Opacity Row
        add_child(create_slider_row(i18n().tr("preferences.appearance.panel_opacity"),
                                    m_panel_opacity, &m_panel_opacity_slider,
                                    &m_panel_opacity_label));

        // Menu Opacity Row
        add_child(create_slider_row(i18n().tr("preferences.appearance.menu_opacity"),
                                    m_menu_opacity, &m_menu_opacity_slider, &m_menu_opacity_label));

        update_selection_visuals();
    }

    void AppearanceView::update_selection_visuals()
    {
        if (m_light_box)
        {
            m_light_box->set_border_width(m_variant == "light" ? 3 : 1);
            m_light_box->set_border_color(m_variant == "light" ? Color(0.12f, 0.3f, 0.88f)
                                                               : Color(0.5f, 0.5f, 0.5f, 0.3f));
        }
        if (m_dark_box)
        {
            m_dark_box->set_border_width(m_variant == "dark" ? 3 : 1);
            m_dark_box->set_border_color(m_variant == "dark" ? Color(0.12f, 0.3f, 0.88f)
                                                             : Color(0.5f, 0.5f, 0.5f, 0.3f));
        }
        if (m_compositor_checkbox)
        {
            m_compositor_checkbox->set_checked(m_use_effects);
        }
    }

    void AppearanceView::load_compositor_config()
    {
        m_use_effects = false;
        std::string config_path = std::string(getenv("HOME")) + "/.config/meteor.ini";
        std::ifstream f(config_path);
        if (f.is_open())
        {
            std::string line;
            bool in_core = false;
            bool in_plugins = false;
            while (std::getline(f, line))
            {
                if (!line.empty() && line[0] == '[') {
                    in_core = (line.find("[core]") != std::string::npos);
                    in_plugins = false;
                }
                
                if (in_core) {
                    if (line.find("plugins") != std::string::npos && line.find("=") != std::string::npos) {
                        in_plugins = true;
                    }
                    if (in_plugins) {
                        if (line.find("wobbly") != std::string::npos || line.find("blur") != std::string::npos) {
                            m_use_effects = true;
                            break;
                        }
                        if (line.empty() || line.back() != '\\') {
                            in_plugins = false; // end of plugins list
                        }
                    }
                }
            }
        }
    }

    void AppearanceView::set_use_effects(bool use_effects)
    {
        if (use_effects == m_use_effects)
            return;

        m_use_effects = use_effects;
        
        std::string config_path = std::string(getenv("HOME")) + "/.config/meteor.ini";
        
        // Ensure config exists
        if (!std::filesystem::exists(config_path)) {
            std::error_code ec;
            std::filesystem::copy_file("/usr/share/horizon/meteor.ini", config_path, std::filesystem::copy_options::overwrite_existing, ec);
        }

        std::vector<std::string> lines;
        std::ifstream f(config_path);
        bool in_core = false;
        bool skipping_plugins = false;

        std::string basic_plugins = "autostart command core decoration grid move place resize switcher window-rules wm-actions wproto foreign-toplevel";
        std::string fx_plugins = "alpha animate expo vswitch wobbly blur winshadows";

        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                if (skipping_plugins) {
                    if (!line.empty() && line.back() == '\\') {
                        continue;
                    } else {
                        skipping_plugins = false;
                        continue;
                    }
                }

                if (!line.empty() && line[0] == '[') {
                    in_core = (line.find("[core]") != std::string::npos);
                }

                if (in_core && line.find("plugins") != std::string::npos && line.find("=") != std::string::npos) {
                    std::string new_line = "plugins = " + basic_plugins;
                    if (use_effects) {
                        new_line += " " + fx_plugins;
                    }
                    lines.push_back(new_line);
                    
                    if (!line.empty() && line.back() == '\\') {
                        skipping_plugins = true;
                    }
                } else {
                    lines.push_back(line);
                }
            }
            f.close();

            std::ofstream out(config_path);
            for (const auto& l : lines) {
                out << l << "\n";
            }
        }

        update_selection_visuals();
        
        if (m_restart_hint_label)
        {
            m_restart_hint_label->set_text(
                i18n().tr("preferences.appearance.compositor_restart_hint"));
        }
    }

    void AppearanceView::from_json(const nlohmann::json &j)
    {
        if (j.is_string())
        {
            m_variant = j.get<std::string>();
        }
    }

    nlohmann::json AppearanceView::to_json() const
    {
        return m_variant;
    }

    void AppearanceView::save_config()
    {
        if (m_is_loading)
            return;

        m_config->set_section("variant", to_json());

        if (m_panel_opacity_slider)
        {
            m_panel_opacity = m_panel_opacity_slider->value() / 100.0f;
            m_config->set_section("panel_opacity", m_panel_opacity);
        }

        if (m_menu_opacity_slider)
        {
            m_menu_opacity = m_menu_opacity_slider->value() / 100.0f;
            m_config->set_section("menu_opacity", m_menu_opacity);
        }

        m_config->save();
    }
} // namespace horizon::preferences
