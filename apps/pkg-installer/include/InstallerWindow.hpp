#pragma once

#include <horizon/Window.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Image.hpp>
#include <horizon/Frame.hpp>
#include <horizon/Label.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/LoadingBar.hpp>
#include "DebInspector.hpp"
#include <memory>

namespace horizon {

class InstallerWindow : public Window {
public:
    InstallerWindow();

    void load_deb(const std::string& path);
    uint32_t file_capabilities() const override { return FileOpen; }

private:
    void setup_ui();
    void start_installation();
    void update_status(const std::string& message, WidgetAccentColor accent = WidgetAccentColor::Default);

    Icon* m_app_icon;
    Icon* m_system_icon;
    Image* m_app_image; // Keep image as fallback for direct file paths
    Label* m_name_label;
    Label* m_desc_label;
    Label* m_feedback_label;
    ProgressBar* m_progress_bar;
    LoadingBar* m_loading_bar;

    std::optional<DebInfo> m_current_deb;
    std::string m_deb_path;
};

} // namespace horizon
