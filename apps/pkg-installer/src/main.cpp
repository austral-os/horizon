#include "InstallerWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

using namespace horizon;

int main(int argc, char **argv)
{
    Application app("pkg-installer", 700, 550);
    i18n().load_app_locales("pkg-installer");

    app.set_name(i18n().tr("app.title"));
    app.set_icon_name("pkg-installer");

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title(i18n().tr("app.title"));
    about.set_app_description(i18n().tr("app.description"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("pkg-installer");

    auto window = std::make_unique<InstallerWindow>();
    auto *window_ptr = window.get();

    // Handle command line argument
    if (argc > 1)
    {
        window_ptr->load_deb(argv[1]);
    }

    app.set_root(std::move(window));
    app.run();

    return 0;
}
