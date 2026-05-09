#pragma once

#include "ITopPanelWidget.hpp"
#include <horizon/Icon.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/FileWatcher.hpp>
#include <memory>

namespace horizon {

class NotificationIndicator : public ITopPanelWidget, public FileWatcher
{
public:
    NotificationIndicator();
    virtual ~NotificationIndicator() override;

    std::string widget_id() const override { return "notifications"; }
    std::string widget_name() const override { return "Notifications"; }

    int preferred_width() const override;

protected:
    // FileWatcher implementation
    void on_file_changed() override;
    void post_watcher_task(std::function<void()> task) override;

private:
    void update_state();
    void toggle_state();

    Icon* m_icon{nullptr};
    std::unique_ptr<ConfigManager> m_config;
    bool m_enabled{true};
};

} // namespace horizon
