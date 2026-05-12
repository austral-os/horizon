
#include "AboutWindow.hpp"
#include <horizon/About.hpp>
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>("org.horizon.aboutus", 1000, 600);

    // Load translations
    horizon::i18n().load_app_locales("aboutus");

    app->set_name(horizon::i18n().tr("aboutus.title"));
    app->set_icon_name("computer");
    
    // Setup About info
    auto &about = app->about_manager();
    about.set_app_title(horizon::i18n().tr("aboutus.title"));
    about.set_app_description(horizon::i18n().tr("aboutus.about"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("computer");
    about.set_app_git(horizon::ABOUT_HORIZON.git);
    about.add_app_translator("Horacio", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com");

    auto window = std::make_unique<AboutWindow>();

    app->set_root(std::move(window));

    app->run();
    return 0;
}
