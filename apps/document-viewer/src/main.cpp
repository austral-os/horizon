#include "DocumentWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>

int main(int argc, char** argv) {
    horizon::Application app("org.horizon.document-viewer", 1024, 768);
    
    // Configurar i18n
    horizon::i18n().load_app_locales("document-viewer");
    
    auto window = std::make_unique<horizon::pdf::DocumentWindow>();
    
    // Soporte para abrir archivo desde argumentos o por defecto (hardcoded)
    if (argc > 1) {
        window->open_file(argv[1]);
    } else {
        window->open_file("/home/horacio/Descargas/comandos de git.pdf");
    }
    
    app.set_root(std::move(window));
    app.run();
    return 0;
}
