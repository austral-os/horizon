#include "AppStoreWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    auto app = std::make_unique<horizon::Application>("horizon-appstore", 1000, 700);
    
    // Load app-specific locales
    horizon::i18n().load_app_locales("horizon-appstore");

    std::string initial_view = horizon::i18n().tr("appstore.views.featured");
    std::string initial_search = "";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << horizon::i18n().tr("appstore.help.usage") << std::endl;
            return 0;
        } else if (arg == "--explore" || arg == "explore") {
            initial_view = horizon::i18n().tr("appstore.views.explore");
        } else if (arg == "--updates" || arg == "updates") {
            initial_view = horizon::i18n().tr("appstore.views.updates");
        } else if (arg.find("--search=") == 0) {
            initial_search = arg.substr(9);
            initial_view = horizon::i18n().tr("appstore.views.explore");
        } else if (arg == "--search" && i + 1 < argc) {
            initial_search = argv[++i];
            initial_view = horizon::i18n().tr("appstore.views.explore");
        }
    }


    app->set_name("AppStore");
    app->set_icon_name("system-software-install");

    auto &about = app->about_manager();
    about.set_app_title("AppStore");
    about.set_app_description(horizon::i18n().tr("appstore.about.description"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("system-software-install");

    auto window = std::make_unique<horizon::appstore::AppStoreWindow>(initial_view, initial_search);
    app->set_root(std::move(window));
    
    app->run();
    
    return 0;
}
