#include "ImageViewerWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char** argv) {
    horizon::Application app("image-viewer", 1024, 768);
    horizon::i18n().load_app_locales("image-viewer");
    
    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Image Viewer");
    about.set_app_description("A fast and versatile image viewer for Horizon.");
    about.set_app_version("0.1.0");
    about.set_app_icon("image-viewer");
    about.set_app_git(horizon::ABOUT_HORIZON.git);
    about.add_app_author("Horacio Daniel Ros", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com");
    about.add_app_translator("Horacio Daniel Ros", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com");
    
    auto window = std::make_unique<horizon::image::ImageViewerWindow>();
    
    if (argc > 1) {
        // Abrir cada archivo pasado como argumento en una pestaña
        for (int i = 1; i < argc; ++i) {
            window->open_file(argv[i]);
        }
    } else {
        window->open_file("/home/horacio/Desarrollo/austral-os/horizon/examples/img-stock/1.png");
    }
    
    app.set_root(std::move(window));
    app.run();
    return 0;
}
