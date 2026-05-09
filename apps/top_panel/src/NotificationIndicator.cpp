#include "NotificationIndicator.hpp"
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Notification.hpp>
#include <filesystem>

namespace horizon {

NotificationIndicator::NotificationIndicator() : ITopPanelWidget()
{
    auto icon = std::make_unique<Icon>();
    icon->set_icon_name("notifications-active-symbolic");
    m_icon = icon.get();
    m_icon->set_fixed_size(20);
    m_icon->set_vertical_alignment(VerticalAlignment::Middle);
    add_child(std::move(icon));

    set_margin(5);

    char *home = getenv("HOME");
    std::string config_path = home ? std::string(home) + "/.config/horizon/notifications.json" : "notifications.json";
    m_config = std::make_unique<ConfigManager>(config_path);

    this->when_mouse_press.connect([this](MouseButtonEventContext &ctx) {
        if (ctx.button == 0x110) { // Left click
            this->toggle_state();
        }
    });

    when_application_load.connect([this, config_path](EventContext &) {
        this->update_state();
        this->start_watching(config_path);
    });
}

NotificationIndicator::~NotificationIndicator()
{
    stop_watching();
}

void NotificationIndicator::update_state()
{
    if (m_config->load()) {
        m_enabled = m_config->get_value("general", "enabled", true).get<bool>();
    } else {
        m_enabled = true;
    }

    if (m_enabled) {
        m_icon->set_icon_name("notifications-active-symbolic");
        
        auto tip = std::make_unique<Notification>();
        tip->set_notification("notifications-active", "Notificaciones activadas");
        set_tooltip(std::move(tip));
    } else {
        m_icon->set_icon_name("notifications-disabled-symbolic");
        
        auto tip = std::make_unique<Notification>();
        tip->set_notification("notifications-disabled", "Notificaciones silenciadas");
        set_tooltip(std::move(tip));
    }
    
    invalidate();
}

void NotificationIndicator::toggle_state()
{
    m_enabled = !m_enabled;
    m_config->set_value("general", "enabled", m_enabled);
    m_config->save();
    
    // update_state will be called via FileWatcher anyway, but let's do it now for instant feedback
    update_state();
}

void NotificationIndicator::on_file_changed()
{
    update_state();
}

void NotificationIndicator::post_watcher_task(std::function<void()> task)
{
    if (application()) {
        application()->post_task(task);
    }
}

int NotificationIndicator::preferred_width() const
{
    return m_icon->preferred_width() + (margin() * 2);
}

} // namespace horizon
