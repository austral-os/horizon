#include "KeyringWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

using namespace horizon;
using namespace horizon::keyring;

int main(int argc, char **argv)
{
    // Application constructor: (app_id, width, height)
    Application app("horizon.keyring.manager", 900, 600);

    // Load translations
    i18n().load_app_locales("keyring-manager");

    app.set_name(i18n().tr("keyring.title"));
    app.set_icon_name("dialog-password");

    app.about_manager().set_app_title(i18n().tr("keyring.title"));
    app.about_manager().set_app_description("Manage your system passwords and keys securely.");
    app.about_manager().set_app_version("0.1.0");
    app.about_manager().set_app_icon("dialog-password");

    auto window = std::make_unique<KeyringWindow>(900, 600);
    app.set_root(std::move(window));

    app.run();
    return 0;
}
