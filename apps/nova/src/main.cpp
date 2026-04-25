#include "horizon/Application.hpp"
#include "BrowserWindow.hpp"
#include "horizon/web/WebView.hpp"
#include "horizon/I18n.hpp"

using namespace horizon;
using namespace horizon::nova;

int main(int argc, char** argv) {
    Application app("org.horizon.nova", 1024, 768);

    // Load translations
    horizon::i18n().load_app_locales("nova");

    app.set_name(horizon::i18n().tr("nova.title"));
    app.set_icon_name("web-browser");

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Nova");
    about.set_app_description("Nova is a fast and lightweight web browser for Horizon.");
    about.set_app_version("0.1.0");
    about.set_app_icon("web-browser");
    
    std::string initial_url = "";
    if (argc > 1) {
        initial_url = argv[1];
    }
    
    auto browser_window = std::make_unique<BrowserWindow>(initial_url);
    
    app.set_root(std::move(browser_window));
    
    app.run();

    // Force destruction of UI components while the worker thread is still alive
    app.set_root(nullptr);

    horizon::web::WebView::shutdown();
    
    return 0;
}
