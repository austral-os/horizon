#include <ConfigManager.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <views/DesktopView/DockView.hpp>

namespace horizon::preferences
{
    DockView::DockView() : horizon::Widget()
    {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::WidgetPositionTypes::FILL);
        set_margin(30);
        set_spacing(20);

        // --- Icon Size Section ---
        auto size_container = std::make_unique<horizon::Widget>();
        size_container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        size_container->set_spacing(10);

        auto size_title = std::make_unique<horizon::Label>("Tamaño de Iconos");
        size_container->add_child(std::move(size_title));

        auto slider_hbox = std::make_unique<horizon::Widget>();
        slider_hbox->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        slider_hbox->set_spacing(15);
        slider_hbox->set_height(40);

        auto slider = std::make_unique<horizon::Slider>();
        slider->set_orientation(horizon::SliderOrientation::Horizontal);
        slider->set_min(32.0f);
        slider->set_max(128.0f);
        slider->set_value(64.0f);
        slider->set_width(300);
        m_size_slider = slider.get();

        m_size_slider->when_value_changed.connect(
            [this](const horizon::EventContext &)
            {
                float val = m_size_slider->value();
                m_icon_size = static_cast<int>(val);
                m_size_label->set_text(std::to_string(m_icon_size) + " px");
                save_config();
            });

        slider_hbox->add_child(std::move(slider));

        auto size_val_label = std::make_unique<horizon::Label>("64 px");
        m_size_label = size_val_label.get();
        slider_hbox->add_child(std::move(size_val_label));
        slider_hbox->add_child(horizon::Spacer());

        size_container->add_child(std::move(slider_hbox));
        add_child(std::move(size_container));

        // --- Magnification Section ---
        auto mag_container = std::make_unique<horizon::Widget>();
        mag_container->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        mag_container->set_height(40);

        auto mag_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        mag_check->set_text("Usar Magnificación");
        mag_check->set_width(250);
        m_magnification_check = mag_check.get();

        m_magnification_check->set_on_toggle(
            [this](bool checked)
            {
                m_magnification_enabled = checked;
                save_config();
            });

        mag_container->add_child(std::move(mag_check));
        mag_container->add_child(horizon::Spacer());

        add_child(std::move(mag_container));
        add_child(horizon::Spacer());

        // Load configuration
        from_json(ConfigManager::instance().get_section("dock"));
    }

    void DockView::from_json(const nlohmann::json &j)
    {
        if (j.is_null())
            return;

        m_icon_size = j.value("icon_size", 64);
        m_magnification_enabled = j.value("magnification_enabled", true);

        if (m_size_slider)
            m_size_slider->set_value(static_cast<float>(m_icon_size));

        if (m_size_label)
            m_size_label->set_text(std::to_string(m_icon_size) + " px");

        if (m_magnification_check)
            m_magnification_check->set_checked(m_magnification_enabled);
    }

    nlohmann::json DockView::to_json() const
    {
        return {{"icon_size", m_icon_size}, {"magnification_enabled", m_magnification_enabled}};
    }

    void DockView::save_config()
    {
        ConfigManager::instance().set_section("dock", to_json());
        ConfigManager::instance().save();
    }
} // namespace horizon::preferences
