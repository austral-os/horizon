#include "WelcomeWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>("org.austral.welcome", 950, 600);

    // Load translations
    horizon::i18n().load_app_locales("austral-welcome");

    app->set_name(horizon::i18n().tr("welcome.title"));
    app->set_icon_name("emblem-austral");

    // Setup About info
    auto &about = app->about_manager();
    about.set_app_title(horizon::i18n().tr("welcome.title"));
    about.set_app_description(horizon::i18n().tr("welcome.description"));
    about.set_app_version("0.1.0");
    about.set_app_icon("emblem-austral");

    auto window = std::make_unique<WelcomeWindow>();

    app->set_root(std::move(window));

    app->run();
    return 0;
}
