#include <ConfigManager.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <views/DesktopView/DockView.hpp>
#include <horizon/I18n.hpp>

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

        auto size_title = std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.icon_size"));
        size_container->add_child(std::move(size_title));

        auto slider = std::make_unique<horizon::Slider>();
        slider->set_orientation(horizon::SliderOrientation::Horizontal);
        slider->set_min(32.0f);
        slider->set_max(128.0f);
        slider->set_value(64.0f);
        slider->set_width(300);
        m_size_slider = slider.get();
        m_size_slider->set_show_ticks(true);
        m_size_slider->add_custom_tick(48.0f);
        m_size_slider->add_custom_tick(64.0f);
        m_size_slider->add_custom_tick(72.0f);
        m_size_slider->add_custom_tick(96.0f);

        m_size_slider->when_value_changed.connect(
            [this](const horizon::EventContext &)
            {
                float val = m_size_slider->value();
                m_icon_size = static_cast<int>(val);
                m_size_label->set_text(std::to_string(m_icon_size) + " px");
            });

        m_size_slider->when_changed.connect(
            [this](const horizon::EventContext &)
            {
                save_config();
            });

        size_container->add_child(std::move(slider));

        auto size_val_label = std::make_unique<horizon::Label>("64 px");
        m_size_label = size_val_label.get();
        size_container->add_child(std::move(size_val_label));
        size_container->add_child(horizon::Spacer());

        auto mag_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        mag_check->set_text(i18n().tr("preferences.desktop.use_magnification"));
        mag_check->set_fixed_size(25);
        m_magnification_check = mag_check.get();

        m_magnification_check->set_on_toggle(
            [this](bool checked)
            {
                m_magnification_enabled = checked;
                save_config();
            });

        size_container->add_child(std::move(mag_check));
        size_container->add_child(horizon::Spacer());

        add_child(std::move(size_container));

        add_child(horizon::Spacer());

        // Load configuration
        from_json(ConfigManager::instance().get_section("dock"));
    }

    void DockView::from_json(const nlohmann::json &j)
    {
        if (j.is_null())
            return;

        m_config_data = j;
        m_icon_size = j.value("icon_size", 64);
        m_magnification_enabled = j.value("magnification_enabled", j.value("magnification", true));

        if (m_size_slider)
            m_size_slider->set_value(static_cast<float>(m_icon_size));

        if (m_size_label)
            m_size_label->set_text(std::to_string(m_icon_size) + " px");

        if (m_magnification_check)
            m_magnification_check->set_checked(m_magnification_enabled);
    }

    nlohmann::json DockView::to_json() const
    {
        nlohmann::json j = m_config_data;
        if (j.is_null())
            j = nlohmann::json::object();

        j["icon_size"] = m_icon_size;
        j["magnification_enabled"] = m_magnification_enabled;
        return j;
    }

    void DockView::save_config()
    {
        ConfigManager::instance().set_section("dock", to_json());
        ConfigManager::instance().save();
    }
} // namespace horizon::preferences
