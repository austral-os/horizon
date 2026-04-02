#include <views/KeyboardView/KeyboardHardwareView.hpp>
#include <utils/XkbParser.hpp>
#include <horizon/VPanel.hpp>

namespace horizon::preferences
{
    KeyboardHardwareView::KeyboardHardwareView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(20);

        // --- Keyboard Model Selection ---
        auto model_section = std::make_unique<Widget>();
        model_section->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        model_section->set_fixed_size(60);
        model_section->set_spacing(5);

        auto model_label = std::make_unique<Label>("Modelo de Teclado");
        model_label->set_fixed_size(20);
        model_section->add_child(std::move(model_label));

        auto model_combo = std::make_unique<Combo>();
        model_combo->set_fixed_size(30);
        m_model_combo = model_combo.get();
        
        // Populate models
        auto models = XkbParser::get_models();
        for (const auto& model : models)
        {
            m_model_combo->add_item(model.id, model.description);
        }
        
        if (!models.empty())
        {
            m_model_combo->set_selected_item_by_id("pc105"); // Default or first
        }

        model_section->add_child(std::move(model_combo));
        add_child(std::move(model_section));

        // --- First Row: Sliders (Horizontal Container) ---
        auto sliders_row = std::make_unique<Widget>();
        sliders_row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        sliders_row->set_position_type(WidgetPositionTypes::FILL);
        sliders_row->set_spacing(40);
        sliders_row->set_fixed_size(80);

        // Delay Section
        auto delay_container = std::make_unique<Widget>();
        delay_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        delay_container->set_position_type(WidgetPositionTypes::FILL);
        delay_container->set_spacing(0);

        auto delay_label = std::make_unique<Label>("Delay");
        delay_label->set_fixed_size(20);
        delay_container->add_child(std::move(delay_label));

        auto delay_slider = std::make_unique<Slider>();
        delay_slider->set_fixed_size(35);
        delay_slider->set_min(100.0f);
        delay_slider->set_max(1000.0f);
        delay_slider->set_value(500.0f);
        delay_slider->set_tick_count(3);
        delay_slider->set_show_ticks(true);
        m_delay_slider = delay_slider.get();
        delay_container->add_child(std::move(delay_slider));

        // Delay Markers Row
        auto delay_markers = std::make_unique<Widget>();
        delay_markers->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        delay_markers->set_fixed_size(15);
        delay_markers->set_margin(12); // Match TRACK_PAD

        auto delay_min = std::make_unique<Label>("100ms");
        delay_min->set_alignment(TextAlignment::Left);
        delay_markers->add_child(std::move(delay_min));

        auto delay_mid = std::make_unique<Label>("500ms");
        delay_mid->set_alignment(TextAlignment::Center);
        delay_markers->add_child(std::move(delay_mid));

        auto delay_max = std::make_unique<Label>("1000ms");
        delay_max->set_alignment(TextAlignment::Right);
        delay_markers->add_child(std::move(delay_max));

        delay_container->add_child(std::move(delay_markers));

        sliders_row->add_child(std::move(delay_container));

        // Rate Section
        auto rate_container = std::make_unique<Widget>();
        rate_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        rate_container->set_position_type(WidgetPositionTypes::FILL);
        rate_container->set_spacing(0);

        auto rate_label = std::make_unique<Label>("Rate");
        rate_label->set_fixed_size(20);
        rate_container->add_child(std::move(rate_label));

        auto rate_slider = std::make_unique<Slider>();
        rate_slider->set_fixed_size(35);
        rate_slider->set_min(1.0f);
        rate_slider->set_max(50.0f);
        rate_slider->set_value(25.0f);
        rate_slider->set_tick_count(3);
        rate_slider->set_show_ticks(true);
        m_rate_slider = rate_slider.get();
        rate_container->add_child(std::move(rate_slider));

        // Rate Markers Row
        auto rate_markers = std::make_unique<Widget>();
        rate_markers->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        rate_markers->set_fixed_size(15);
        rate_markers->set_margin(12); // Match TRACK_PAD

        auto rate_min = std::make_unique<Label>("1Hz");
        rate_min->set_alignment(TextAlignment::Left);
        rate_markers->add_child(std::move(rate_min));

        auto rate_mid = std::make_unique<Label>("25Hz");
        rate_mid->set_alignment(TextAlignment::Center);
        rate_markers->add_child(std::move(rate_mid));

        auto rate_max = std::make_unique<Label>("50Hz");
        rate_max->set_alignment(TextAlignment::Right);
        rate_markers->add_child(std::move(rate_max));

        rate_container->add_child(std::move(rate_markers));

        sliders_row->add_child(std::move(rate_container));

        add_child(std::move(sliders_row));

        // --- Second Row: Checkbox ---
        auto numlock_check = std::make_unique<Checkbox<AquaObject>>();
        numlock_check->set_text("Iniciar Num Lock activado");
        numlock_check->set_fixed_size(30);
        m_numlock_checkbox = numlock_check.get();
        add_child(std::move(numlock_check));

        // --- Bottom Spacer ---
        add_child(Spacer());
    }
} // namespace horizon::preferences
