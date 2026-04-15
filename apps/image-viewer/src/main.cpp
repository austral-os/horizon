#include "ImageViewerWindow.hpp"
#include <horizon/Application.hpp>

int main(int argc, char** argv) {
    horizon::Application app("org.horizon.image-viewer", 1024, 768);
    
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
