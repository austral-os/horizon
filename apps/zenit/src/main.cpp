#include "ZenitWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("org.austral.horizon.zenit", 1280, 720);

    horizon::i18n().load_app_locales("zenit");

    app.set_icon_name("zenit");
    app.set_name(horizon::i18n().tr("zenit.title"));

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title(horizon::i18n().tr("zenit.title"));
    about.set_app_description(horizon::i18n().tr("zenit.description"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("zenit");
    about.set_app_git("https://github.com/austral-os/horizon");
    about.add_app_author("Austral OS Team", "https://github.com/austral-os/horizon");

    auto win = std::make_unique<zenit::ZenitWindow>();
    auto *win_ptr = win.get();

    if (argc > 1)
    {
        win_ptr->open_file(argv[1]);
    }

    app.set_root(std::move(win));
    app.run();

    return 0;
}
