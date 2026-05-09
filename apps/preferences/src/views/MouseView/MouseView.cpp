#include <views/MouseView/MouseView.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Frame.hpp>
#include <horizon/Widget.hpp>
#include <utils/ConfigUtils.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <regex>

namespace horizon::preferences
{
    MouseView::MouseView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(20);

        m_config = std::make_unique<ConfigManager>(get_config_path("mouse.json"));
        m_config->load();

        setup_ui();
        load_config();
    }

    void MouseView::setup_ui()
    {

        auto container = std::make_unique<Frame>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(0);
        container->set_spacing(20);

        // Content Area
        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_position_type(WidgetPositionTypes::FILL);
        content->set_margin(30);
        content->set_spacing(30);

        // Title
        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.mouse"));
        title->set_font_size(18);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_fixed_size(40);
        m_title_label = title.get();
        content->add_child(std::move(title));

        // --- Double Click Speed ---
        auto dc_section = std::make_unique<Widget>();
        dc_section->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        dc_section->set_spacing(10);

        auto dc_label = std::make_unique<Label>(
            i18n().tr("preferences.sections.mouse_settings.double_click_speed"));
        dc_label->set_fixed_size(20);
        dc_section->add_child(std::move(dc_label));

        auto dc_slider_row = std::make_unique<Widget>();
        dc_slider_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        dc_slider_row->set_spacing(10);
        dc_slider_row->set_fixed_size(30);

        auto dc_slow =
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.fast"));
        dc_slow->set_font_size(11);
        dc_slow->set_fixed_size(50);
        dc_slider_row->add_child(std::move(dc_slow));

        auto dc_slider = std::make_unique<Slider>();
        m_double_click_slider = dc_slider.get();
        m_double_click_slider->set_min(250.0f);
        m_double_click_slider->set_max(1000.0f);
        m_double_click_slider->set_value(500.0f);
        m_double_click_slider->set_position_type(WidgetPositionTypes::FILL);
        m_double_click_slider->when_changed.connect([this](EventContext &) { save_config(); });
        dc_slider_row->add_child(std::move(dc_slider));

        auto dc_fast =
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.slow"));
        dc_fast->set_font_size(11);
        dc_fast->set_fixed_size(50);
        dc_slider_row->add_child(std::move(dc_fast));

        dc_section->add_child(std::move(dc_slider_row));
        content->add_child(std::move(dc_section));

        // --- Pointer Speed ---
        auto ps_section = std::make_unique<Widget>();
        ps_section->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        ps_section->set_spacing(10);

        auto ps_label =
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.pointer_speed"));
        ps_label->set_fixed_size(20);
        ps_section->add_child(std::move(ps_label));

        auto ps_slider_row = std::make_unique<Widget>();
        ps_slider_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        ps_slider_row->set_spacing(10);
        ps_slider_row->set_fixed_size(30);

        auto ps_slow =
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.slow"));
        ps_slow->set_font_size(11);
        ps_slow->set_fixed_size(50);
        ps_slider_row->add_child(std::move(ps_slow));

        auto ps_slider = std::make_unique<Slider>();
        m_pointer_speed_slider = ps_slider.get();
        m_pointer_speed_slider->set_min(-1.0f);
        m_pointer_speed_slider->set_max(1.0f);
        m_pointer_speed_slider->set_value(0.0f);
        m_pointer_speed_slider->set_position_type(WidgetPositionTypes::FILL);
        m_pointer_speed_slider->when_changed.connect([this](EventContext &) { save_config(); });
        ps_slider_row->add_child(std::move(ps_slider));

        auto ps_fast =
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.fast"));
        ps_fast->set_font_size(11);
        ps_fast->set_fixed_size(50);
        ps_slider_row->add_child(std::move(ps_fast));

        ps_section->add_child(std::move(ps_slider_row));
        content->add_child(std::move(ps_section));

        content->add_child(Spacer());
        container->add_child(std::move(content));

        add_child(std::move(container));
    }

    void MouseView::load_config()
    {
        auto j = m_config->get_section("mouse");
        if (j.is_null())
            return;

        if (j.contains("double_click_speed") && m_double_click_slider)
        {
            m_double_click_slider->set_value(j["double_click_speed"].get<float>());
        }

        if (j.contains("pointer_speed") && m_pointer_speed_slider)
        {
            m_pointer_speed_slider->set_value(j["pointer_speed"].get<float>());
        }
    }

    void MouseView::save_config()
    {
        nlohmann::json j;
        if (m_double_click_slider)
        {
            j["double_click_speed"] = m_double_click_slider->value();
        }
        if (m_pointer_speed_slider)
        {
            j["pointer_speed"] = m_pointer_speed_slider->value();
        }

        m_config->set_section("mouse", j);
        m_config->save();

        if (m_pointer_speed_slider)
        {
            float speed = m_pointer_speed_slider->value();
            apply_to_labwc(speed);
            apply_to_wayfire(speed);
        }
    }

    void MouseView::apply_to_labwc(float speed)
    {
        const char *home = std::getenv("HOME");
        if (!home) return;

        std::filesystem::path config_path(home);
        config_path /= ".config/labwc/rc.xml";

        if (!std::filesystem::exists(config_path)) return;

        std::ifstream file(config_path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        // Very basic XML "parsing" using regex to update accel_speed
        // This is not perfect for all XML cases but should work for standard rc.xml
        std::regex accel_regex("<accel_speed>.*</accel_speed>");
        std::string new_val = "<accel_speed>" + std::to_string(speed) + "</accel_speed>";

        if (std::regex_search(content, accel_regex)) {
            content = std::regex_replace(content, accel_regex, new_val);
        } else {
            // Try to insert it into <libinput><device> or at the end of <labwc_config>
            std::regex device_regex("<device>");
            if (std::regex_search(content, device_regex)) {
                content = std::regex_replace(content, device_regex, "<device>\n    " + new_val);
            } else {
                std::regex libinput_regex("<libinput>");
                if (std::regex_search(content, libinput_regex)) {
                    content = std::regex_replace(content, libinput_regex, "<libinput>\n  <device>\n    " + new_val + "\n  </device>");
                } else {
                    std::regex root_regex("</labwc_config>");
                    content = std::regex_replace(content, root_regex, "  <libinput>\n    <device>\n      " + new_val + "\n    </device>\n  </libinput>\n</labwc_config>");
                }
            }
        }

        std::ofstream out(config_path);
        out << content;
        out.close();

        // Check if we are in labwc to run reconfigure
        const char *desktop_env = getenv("XDG_CURRENT_DESKTOP");
        std::string desktop_str = desktop_env ? desktop_env : "";
        std::transform(desktop_str.begin(), desktop_str.end(), desktop_str.begin(), ::tolower);
        if (desktop_str.find("labwc") != std::string::npos) {
            std::system("labwc --reconfigure");
        }
    }

    void MouseView::apply_to_wayfire(float speed)
    {
        const char *home = std::getenv("HOME");
        if (!home) return;

        std::filesystem::path config_path(home);
        config_path /= ".config/wayfire.ini";

        if (!std::filesystem::exists(config_path)) return;

        std::vector<std::string> lines;
        bool in_input_section = false;
        bool speed_found = false;
        std::string speed_str = "mouse_cursor_speed = " + std::to_string(speed);

        std::ifstream file(config_path);
        std::string line;
        while (std::getline(file, line)) {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

            if (trimmed == "[input]") {
                in_input_section = true;
            } else if (!trimmed.empty() && trimmed[0] == '[' && trimmed.back() == ']') {
                if (in_input_section && !speed_found) {
                    lines.push_back(speed_str);
                    speed_found = true;
                }
                in_input_section = false;
            }

            if (in_input_section && trimmed.compare(0, 19, "mouse_cursor_speed") == 0) {
                lines.push_back(speed_str);
                speed_found = true;
            } else {
                lines.push_back(line);
            }
        }
        file.close();

        if (in_input_section && !speed_found) {
            lines.push_back(speed_str);
            speed_found = true;
        }

        if (!speed_found) {
            lines.push_back("");
            lines.push_back("[input]");
            lines.push_back(speed_str);
        }

        std::ofstream out(config_path);
        for (const auto &l : lines) {
            out << l << "\n";
        }
        out.close();
    }
} // namespace horizon::preferences
