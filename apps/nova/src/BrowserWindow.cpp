#include "BrowserWindow.hpp"
#include "horizon/Button.hpp"
#include "horizon/AquaObject.hpp"
#include "horizon/Logger.hpp"
#include <memory>

namespace horizon {
namespace nova {

BrowserWindow::BrowserWindow() : ApplicationWindow("Nova Web Browser") {
    set_size(1024, 768);
    setup_ui();
}

void BrowserWindow::setup_ui() {
    // 1. Navigation Buttons in Toolbar
    auto back_btn = std::make_unique<Button<AquaObject>>();
    back_btn->set_text("<");
    back_btn->set_fixed_size(32);
    back_btn->when_click.connect([this](auto&) { if (m_web_view) m_web_view->go_back(); });
    toolbar()->add_toolbar_widget(std::move(back_btn));

    auto forward_btn = std::make_unique<Button<AquaObject>>();
    forward_btn->set_text(">");
    forward_btn->set_fixed_size(32);
    forward_btn->when_click.connect([this](auto&) { if (m_web_view) m_web_view->go_forward(); });
    toolbar()->add_toolbar_widget(std::move(forward_btn));

    auto reload_btn = std::make_unique<Button<AquaObject>>();
    reload_btn->set_text("R");
    reload_btn->set_fixed_size(32);
    reload_btn->when_click.connect([this](auto&) { if (m_web_view) m_web_view->reload(); });
    toolbar()->add_toolbar_widget(std::move(reload_btn));

    // 2. Address Bar
    auto address_bar = std::make_unique<TextBox<>>();
    m_address_bar = address_bar.get();
    m_address_bar->set_placeholder("Enter URL or search...");
    m_address_bar->set_position_type(FILL);
    
    // Trigger navigation on Enter
    m_address_bar->when_key_press.connect([this](KeyEventContext& ctx) {
        // Log keysym to help debug if Enter (0xFF0D) isn't matched
        LOG_INFO << "[NOVA] Key press in address bar: 0x" << std::hex << ctx.keysym << std::dec;
        
        if (ctx.keysym == 0xff0d || ctx.keysym == 0xff8d) { // GDK_KEY_Return or GDK_KEY_KP_Enter
             this->navigate_to_url();
        }
    });
    
    toolbar()->add_toolbar_widget(std::move(address_bar));

    // 3. Web View in Main Content area
    auto web_view = std::make_unique<web::WebWidget>();
    m_web_view = web_view.get();
    m_web_view->set_position_type(FILL);
    
    // Connect initial load to the application load signal
    when_application_load.connect([this](auto&) {
        if (m_web_view) {
            m_web_view->load_url("https://www.google.com");
        }
    });

    set_content(std::move(web_view));
}

void BrowserWindow::navigate_to_url() {
    if (m_address_bar && m_web_view) {
        std::string url = m_address_bar->text();
        if (url.empty()) return;
        
        // Basic URL normalization
        if (url.find("://") == std::string::npos) {
            // Check if it looks like a domain or search query
            if (url.find(".") != std::string::npos && url.find(" ") == std::string::npos) {
                url = "https://" + url;
            } else {
                // Should probably be a search query, but for now just add https
                url = "https://www.google.com/search?q=" + url;
            }
        }
        
        LOG_INFO << "[NOVA] Navigating to: " << url;
        m_web_view->load_url(url);
    }
}

} // namespace nova
} // namespace horizon
