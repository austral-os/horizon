#include <horizon/I18n.hpp>
#include <horizon/UnderConstruction.hpp>
#include <views/NotificationsView/NotificationsView.hpp>

namespace horizon::preferences
{
    NotificationsView::NotificationsView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(20);
        set_spacing(10);

        // 1. Initialize Config
        char *home = getenv("HOME");
        std::string config_path = home ? std::string(home) + "/.config/horizon/notifications.json" : "notifications.json";
        m_config = std::make_unique<ConfigManager>(config_path);
        m_config->load();

        bool enabled = m_config->get_value("general", "enabled", true).get<bool>();

        // 2. UI Elements
        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.notifications"));
        title->set_fixed_size(30);
        m_title_label = title.get();
        add_child(std::move(title));

        auto checkbox = std::make_unique<Checkbox<SolidObject>>();
        checkbox->set_text("Mostrar notificaciones");
        checkbox->set_checked(enabled);
        m_enable_checkbox = checkbox.get();

        m_enable_checkbox->when_toggle.connect([this](ToggleEventContext &ctx) {
            m_config->set_value("general", "enabled", ctx.checked);
            m_config->save();
        });

        add_child(std::move(checkbox));
    }
} // namespace horizon::preferences
