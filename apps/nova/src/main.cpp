#include "horizon/Application.hpp"
#include "BrowserWindow.hpp"
#include "horizon/web/WebView.hpp"
#include "horizon/I18n.hpp"
#include "NovaGeneralSection.hpp"
#include "horizon/dialogs/PreferencesContent.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

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
    
    std::string config_path;
    const char* home = getenv("HOME");
    if (home) {
        config_path = std::string(home) + "/.config/horizon/nova.json";
    } else {
        config_path = "nova.json";
    }

    app.set_preferences_content([config_path]() {
        auto content = std::make_unique<PreferencesContent>(config_path);
        auto* content_ptr = content.get();
        
        content->add_section("General", "preferences-system", std::make_unique<NovaGeneralSection>([content_ptr]() {
            content_ptr->save_config();
        }), "general");
        
        return content;
    });

    std::string initial_url = "";
    if (argc > 1) {
        initial_url = argv[1];
    }
    
    auto browser_window = std::make_unique<BrowserWindow>(initial_url);
    
    app.set_root(std::move(browser_window));
    
    app.run();
    
    // Shut down the WebView system before the Application object is destroyed
    horizon::web::WebView::shutdown();
    
    return 0;
}
