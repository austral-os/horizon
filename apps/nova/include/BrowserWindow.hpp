#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "horizon/web/WebWidget.hpp"
#include "horizon/VPanel.hpp"
#include "horizon/Toolbar.hpp"
#include "horizon/TextBox.hpp"

namespace horizon {
namespace nova {

class BrowserWindow : public ApplicationWindow {
public:
    BrowserWindow();
    virtual ~BrowserWindow() = default;

private:
    void setup_ui();
    void navigate_to_url();

    web::WebWidget* m_web_view = nullptr;
    TextBox<>* m_address_bar = nullptr;
};

} // namespace nova
} // namespace horizon
