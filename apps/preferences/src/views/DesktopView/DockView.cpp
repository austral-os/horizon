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
        set_margin(10);
        set_spacing(10);

        // --- Icon Size Section ---
        auto size_container = std::make_unique<horizon::Widget>();
        size_container->set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        size_container->set_fixed_size(96);
        size_container->set_spacing(8);

        auto size_title =
            std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.icon_size"));
        size_title->set_fixed_size(25);
        size_container->add_child(std::move(size_title));

        auto slider = std::make_unique<horizon::Slider>();
        slider->set_orientation(horizon::SliderOrientation::Horizontal);
        slider->set_fixed_size(30);
        slider->set_min(32.0f);
        slider->set_max(128.0f);
        slider->set_value(64.0f);

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
        size_val_label->set_fixed_size(25);
        m_size_label = size_val_label.get();
        size_container->add_child(std::move(size_val_label));

        add_child(std::move(size_container));

        // --- Behavior Section ---
        auto mag_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        mag_check->set_text(i18n().tr("preferences.desktop.use_magnification"));
        mag_check->set_fixed_size(30);
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
        autohide_check->set_fixed_size(30);
        m_autohide_check = autohide_check.get();

        m_autohide_check->when_toggle.connect(
            [this](const ToggleEventContext &ctx)
            {
                m_autohide_enabled = ctx.checked;
                save_config();
            });

        add_child(std::move(autohide_check));

        // --- Position Section ---
        auto pos_row = std::make_unique<horizon::Widget>();
        pos_row->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        pos_row->set_fixed_size(30);
        pos_row->set_spacing(10);

        auto pos_label =
            std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.dock_position"));
        pos_label->set_fixed_size(120);
        pos_row->add_child(std::move(pos_label));

        auto pos_combo = std::make_unique<horizon::Combo>();
        pos_combo->set_fixed_size(250);
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

        pos_row->add_child(std::move(pos_combo));
        pos_row->add_child(horizon::Spacer());

        add_child(std::move(pos_row));

        // --- Applets Section ---
        auto applets_label =
            std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.applets"));
        applets_label->set_fixed_size(25);
        add_child(std::move(applets_label));

        // Downloads: checkbox + count label + input in a row
        auto dl_row = std::make_unique<horizon::Widget>();
        dl_row->set_layout_type(horizon::WIDGET_LAYOUT_HORIZONTAL);
        dl_row->set_fixed_size(30);
        dl_row->set_spacing(10);

        auto dl_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        dl_check->set_text(i18n().tr("preferences.desktop.show_downloads"));
        dl_check->set_fixed_size(350);
        m_show_downloads_check = dl_check.get();
        m_show_downloads_check->when_toggle.connect(
            [this](const ToggleEventContext &ctx)
            {
                m_show_downloads = ctx.checked;
                save_config();
            });
        dl_row->add_child(std::move(dl_check));

        auto dl_count_label =
            std::make_unique<horizon::Label>(i18n().tr("preferences.desktop.downloads_count"));
        dl_count_label->set_fixed_size(200);
        dl_row->add_child(std::move(dl_count_label));

        auto dl_count_input = std::make_unique<horizon::TextBox<horizon::IntegerPolicy>>();
        dl_count_input->config.show_spin_buttons = true;
        dl_count_input->config.min_int = 1;
        dl_count_input->config.max_int = 30;
        dl_count_input->set_fixed_size(80);
        m_downloads_count_input = dl_count_input.get();
        m_downloads_count_input->when_text_changed.connect(
            [this](const horizon::KeyEventContext &)
            {
                if (m_downloads_count_input->text().empty())
                    return;
                try
                {
                    m_downloads_items_count = std::stoi(m_downloads_count_input->text());
                    save_config();
                }
                catch (...)
                {
                }
            });
        dl_row->add_child(std::move(dl_count_input));
        dl_row->add_child(horizon::Spacer());

        add_child(std::move(dl_row));

        // Trash checkbox
        auto trash_check = std::make_unique<horizon::Checkbox<horizon::AquaObject>>();
        trash_check->set_text(i18n().tr("preferences.desktop.show_trash"));
        trash_check->set_fixed_size(30);
        m_show_trash_check = trash_check.get();
        m_show_trash_check->when_toggle.connect(
            [this](const ToggleEventContext &ctx)
            {
                m_show_trash = ctx.checked;
                save_config();
            });
        add_child(std::move(trash_check));

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
        if (j.contains("applets"))
        {
            auto applets = j["applets"];
            m_show_trash = applets.value("show_trash", true);
            m_show_downloads = applets.value("show_downloads", true);
            m_downloads_items_count = applets.value("downloads_items_count", 9);
        }
        else
        {
            m_show_trash = j.value("show_trash", true);
            m_show_downloads = j.value("show_downloads", true);
            m_downloads_items_count = j.value("downloads_items_count", 9);
        }

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

        if (m_show_trash_check)
            m_show_trash_check->set_checked(m_show_trash);

        if (m_show_downloads_check)
            m_show_downloads_check->set_checked(m_show_downloads);

        if (m_downloads_count_input)
            m_downloads_count_input->set_text(std::to_string(m_downloads_items_count));
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

        nlohmann::json applets_json = j.value("applets", nlohmann::json::object());
        applets_json["show_trash"] = m_show_trash;
        applets_json["show_downloads"] = m_show_downloads;
        applets_json["downloads_items_count"] = m_downloads_items_count;
        j["applets"] = applets_json;

        // Clean up legacy flat keys if they exist
        j.erase("show_trash");
        j.erase("show_downloads");
        j.erase("downloads_items_count");

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
