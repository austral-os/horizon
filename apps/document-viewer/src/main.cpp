#include "DocumentWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char **argv)
{
    horizon::Application app("org.horizon.document-viewer", 1024, 768);

    // Configurar i18n
    horizon::I18n::set_search_paths({"/home/horacio/Desarrollo/austral-os/horizon", ".", ".."});
    horizon::i18n().load_app_locales("document-viewer");

    app.set_name(horizon::i18n().tr("app.title"));
    app.set_icon_name("document-viewer");

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title(horizon::i18n().tr("app.title"));
    about.set_app_description("A simple and powerful document viewer for Horizon.");
    about.set_app_version("0.1.0");
    about.set_app_icon("document-viewer");
    about.set_app_git(horizon::ABOUT_HORIZON.git);
    about.add_app_author("Horacio Daniel Ros", "https://github.com/austral-os/horizon",
                         "horaciodrs@gmail.com");
    about.add_app_translator("Horacio Daniel Ros", "https://github.com/austral-os/horizon",
                             "horaciodrs@gmail.com");

    auto window = std::make_unique<horizon::pdf::DocumentWindow>();

    // Soporte para abrir archivo desde argumentos o por defecto (hardcoded)
    if (argc > 1)
    {
        window->open_file(argv[1]);
    }

    app.set_root(std::move(window));
    app.run();
    return 0;
}
