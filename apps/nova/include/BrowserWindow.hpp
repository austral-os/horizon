#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "horizon/web/WebView.hpp"
#include "horizon/VPanel.hpp"
#include "NovaToolbar.hpp"
#include "horizon/Label.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Statusbar.hpp"
#include "horizon/TabCollection.hpp"

namespace horizon {
namespace nova {

class BrowserWindow : public ApplicationWindow {
public:
    BrowserWindow(const std::string& initial_url = "");
    virtual ~BrowserWindow() = default;
    static std::string normalize_url(const std::string& input_url);
    
private:
    void setup_ui(const std::string& initial_url = "");
    void navigate_to_url(const std::string& url);
    void create_new_tab(const std::string& url = "about:blank");
    void load_preferences();

    TabCollection* m_tabs = nullptr;
    NovaToolbar* m_toolbar = nullptr;
    Label* m_status_label = nullptr;
    ProgressBar* m_progress_bar = nullptr;

    std::string m_config_path;
    std::string m_homepage;
};

} // namespace nova
} // namespace horizon
