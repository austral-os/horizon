#include "horizon/Frame.hpp"
#include <horizon/I18n.hpp>
#include <horizon/UnderConstruction.hpp>
#include <views/NotificationsView/NotificationsView.hpp>

namespace horizon::preferences
{
    NotificationsView::NotificationsView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);

        // 1. Initialize Config
        char *home = getenv("HOME");
        std::string config_path =
            home ? std::string(home) + "/.config/horizon/notifications.json" : "notifications.json";
        m_config = std::make_unique<ConfigManager>(config_path);
        m_config->load();

        bool enabled = m_config->get_value("general", "enabled", true).get<bool>();

        auto frame_container = std::make_unique<Frame>();
        frame_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        frame_container->set_margin(20);
        frame_container->set_spacing(10);

        auto container = std::make_unique<Widget>();
        container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        container->set_margin(20);

        // 2. UI Elements
        auto title = std::make_unique<Label>(i18n().tr("preferences.sections.notifications"));
        title->set_fixed_size(30);
        m_title_label = title.get();
        container->add_child(std::move(title));

        auto checkbox = std::make_unique<Checkbox<SolidObject>>();
        checkbox->set_text(i18n().tr("preferences.notifications.show_notifications"));
        checkbox->set_checked(enabled);
        m_enable_checkbox = checkbox.get();

        m_enable_checkbox->when_toggle.connect(
            [this](ToggleEventContext &ctx)
            {
                m_config->set_value("general", "enabled", ctx.checked);
                m_config->save();
            });

        container->add_child(std::move(checkbox));

        frame_container->add_child(std::move(container));

        add_child(std::move(frame_container));
    }
} // namespace horizon::preferences
