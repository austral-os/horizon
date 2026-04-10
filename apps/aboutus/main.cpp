
#include "AboutWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <memory>

using horizon::AboutWindow;
using horizon::Application;

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>("org.horizon.aboutus", 1000, 600);

    // Load translations
    horizon::i18n().load_app_locales("aboutus");

    app->set_name(horizon::i18n().tr("aboutus.title"));
    app->set_icon_name("computer");

    auto window = std::make_unique<AboutWindow>();

    app->set_root(std::move(window));

    app->run();
    return 0;
}
