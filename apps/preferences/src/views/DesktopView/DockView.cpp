#include <horizon/I18n.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Widget.hpp>
#include <utils/ConfigUtils.hpp>
#include <views/DesktopView/DockView.hpp>

namespace horizon::preferences
{
    DockView::DockView() : horizon::Widget()
    {
        m_config = std::make_unique<ConfigManager>(get_config_path("dock.json"));
        m_config->load();
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_position_type(horizon::WidgetPositionTypes::FILL);
        set_margin(30);
        set_spacing(20);

        // --- Icon Size Section ---
        auto size_container = std::make_unique<horizon::Widget>();
        size_container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        size_container->set_spacing(10);
        size_container->set_fixed_size(120);

        auto size_title =
            std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.icon_size"));
        size_title->set_fixed_size(35);
        size_container->add_child(std::move(size_title));

        auto slider = std::make_unique<horizon::Slider>();
        slider->set_fixed_size(35);

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

        m_size_slider->when_changed.connect([this](const horizon::EventContext &)
                                            { save_config(); });

        size_container->add_child(std::move(slider));

        auto size_val_label = std::make_unique<horizon::Label>("64 px");
        // size_val_label->set_fixed_size(35);
        m_size_label = size_val_label.get();
        size_container->add_child(std::move(size_val_label));

        add_child(std::move(size_container));

        auto mag_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        mag_check->set_text(i18n().tr("preferences.desktop.use_magnification"));
        mag_check->set_fixed_size(35);
        m_magnification_check = mag_check.get();

        m_magnification_check->when_toggle.connect(
            [this](const ToggleEventContext &ctx)
            {
                m_magnification_enabled = ctx.checked;
                save_config();
            });

        add_child(std::move(mag_check));

        auto autohide_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        autohide_check->set_text(i18n().tr("preferences.desktop.autohide_dock"));
        autohide_check->set_fixed_size(35);
        m_autohide_check = autohide_check.get();

        m_autohide_check->when_toggle.connect(
            [this](const ToggleEventContext &ctx)
            {
                m_autohide_enabled = ctx.checked;
                save_config();
            });

        add_child(std::move(autohide_check));

        auto pos_label =
            std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.dock_position"));
        pos_label->set_fixed_size(35);
        add_child(std::move(pos_label));

        auto pos_combo = std::make_unique<horizon::Combo>();
        pos_combo->set_fixed_size(35);
        pos_combo->add_item("left", i18n().tr("preferences.desktop.dock_position_left"));
        pos_combo->add_item("bottom", i18n().tr("preferences.desktop.dock_position_bottom"));
        pos_combo->add_item("right", i18n().tr("preferences.desktop.dock_position_right"));

        m_position_combo = pos_combo.get();
        m_position_combo->when_item_selected.connect(
            [this](const horizon::ComboItemSelectedContext &ctx)
            {
                m_position = ctx.item.id;
                save_config();
            });

        add_child(std::move(pos_combo));
        add_child(horizon::Spacer());

        add_child(horizon::Spacer());

        // Load configuration
        from_json(m_config->get_section("dock"));
    }

    void DockView::from_json(const nlohmann::json &j)
    {
        if (j.is_null())
            return;

        m_config_data = j;
        m_icon_size = j.value("icon_size", 64);
        m_magnification_enabled = j.value("magnification_enabled", j.value("magnification", true));
        m_autohide_enabled = j.value("autohide", false);
        m_position = j.value("position", "bottom");

        if (m_size_slider)
            m_size_slider->set_value(static_cast<float>(m_icon_size));

        if (m_size_label)
            m_size_label->set_text(std::to_string(m_icon_size) + " px");

        if (m_magnification_check)
            m_magnification_check->set_checked(m_magnification_enabled);

        if (m_autohide_check)
            m_autohide_check->set_checked(m_autohide_enabled);

        if (m_position_combo)
            m_position_combo->set_selected_item_by_id(m_position);
    }

    nlohmann::json DockView::to_json() const
    {
        nlohmann::json j = m_config_data;
        if (j.is_null())
            j = nlohmann::json::object();

        j["icon_size"] = m_icon_size;
        j["magnification_enabled"] = m_magnification_enabled;
        j["autohide"] = m_autohide_enabled;
        j["position"] = m_position;
        if (!j.contains("autohide-time"))
        {
            j["autohide-time"] = 500;
        }
        return j;
    }

    void DockView::save_config()
    {
        m_config->set_section("dock", to_json());
        m_config->save();
    }
} // namespace horizon::preferences
