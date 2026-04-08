#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "horizon/web/WebWidget.hpp"
#include "horizon/VPanel.hpp"
#include "NovaToolbar.hpp"
#include "horizon/Label.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Statusbar.hpp"

namespace horizon {
namespace nova {

class BrowserWindow : public ApplicationWindow {
public:
    BrowserWindow();
    virtual ~BrowserWindow() = default;

private:
    void setup_ui();
    void navigate_to_url(const std::string& url);

    web::WebWidget* m_web_view = nullptr;
    NovaToolbar* m_toolbar = nullptr;
    Label* m_status_label = nullptr;
    ProgressBar* m_progress_bar = nullptr;
};

} // namespace nova
} // namespace horizon
