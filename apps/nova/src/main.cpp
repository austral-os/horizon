#include "horizon/Application.hpp"
#include "BrowserWindow.hpp"
#include "horizon/web/WebView.hpp"

using namespace horizon;
using namespace horizon::nova;

int main(int argc, char** argv) {
    Application app("org.horizon.nova", 1024, 768);
    app.set_name("Nova");
    app.set_icon_name("web-browser");
    
    auto browser_window = std::make_unique<BrowserWindow>();
    
    app.set_root(std::move(browser_window));
    
    app.run();

    // Force destruction of UI components while the worker thread is still alive
    app.set_root(nullptr);

    horizon::web::WebView::shutdown();
    
    return 0;
}
