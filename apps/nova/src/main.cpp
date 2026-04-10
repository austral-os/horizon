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
    
    auto browser_window = std::make_unique<BrowserWindow>();
    
    app.set_root(std::move(browser_window));
    
    app.run();

    // Force destruction of UI components while the worker thread is still alive
    app.set_root(nullptr);

    horizon::web::WebView::shutdown();
    
    return 0;
}
