#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <utils/ConfigUtils.hpp>
#include <views/MouseView/MouseView.hpp>

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
        // Title
        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.mouse"));
        title->set_font_size(18);
        title->set_font_weight(FONT_WEIGHT_BOLD);
        title->set_fixed_size(40);
        m_title_label = title.get();
        add_child(std::move(title));

        // Content Area
        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_position_type(WidgetPositionTypes::FILL);
        content->set_spacing(30);

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
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.slow"));
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
            std::make_unique<Label>(i18n().tr("preferences.sections.mouse_settings.fast"));
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
        add_child(std::move(content));
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
    }
} // namespace horizon::preferences
