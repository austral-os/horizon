#include "horizon/Application.hpp"
#include "horizon/I18n.hpp"
#include "DownloaderWindow.hpp"

int main(int argc, char** argv) {
    horizon::Application app("org.austral.downloader", 600, 500);
    
    // Load translations
    horizon::i18n().load_app_locales("downloader");

    app.set_icon_name("downloader");

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title(horizon::i18n().tr("downloader.title"));
    about.set_app_description(horizon::i18n().tr("downloader.description"));
    about.set_app_version("0.1.0");
    about.set_app_icon("downloader");

    auto window = std::make_unique<horizon::downloader::DownloaderWindow>();
    app.set_root(std::move(window));
    
    app.run();
    return 0;
}
