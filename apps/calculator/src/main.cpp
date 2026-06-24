#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Window.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("calculator", 600, 800);

    horizon::i18n().load_app_locales("calculator");

    app.set_icon_name("calculator");
    app.set_name(horizon::i18n().tr("calculator.title"));

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title(horizon::i18n().tr("calculator.title"));
    about.set_app_description(horizon::i18n().tr("calculator.description"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("calculator");
    about.set_app_git("https://github.com/austral-os/horizon");
    about.set_app_web("https://australos.hdrdevs.com.ar");
    about.add_app_author("Austral OS Team", "https://github.com/austral-os/horizon");

    auto win = std::make_unique<horizon::Window>(horizon::i18n().tr("calculator.title"));

    app.set_root(std::move(win));
    app.run();

    return 0;
}
