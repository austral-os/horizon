#include "AppStoreWindow.hpp"
#include <horizon/Application.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    std::string initial_view = "Destacados";
    std::string initial_search = "";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Uso: horizon-appstore [OPCIONES]\n\n"
                      << "Opciones:\n"
                      << "  explore                 Inicia en la vista 'Explorar'.\n"
                      << "  --explore               Igual a 'explore'.\n"
                      << "  updates                 Inicia en la vista 'Actualizaciones'.\n"
                      << "  --updates               Igual a 'updates'.\n"
                      << "  --search <query>        Busca un paquete, selecciona y muestra el resultado exacto si lo hay.\n"
                      << "  --search=<query>        Igual a '--search <query>'.\n"
                      << "  -h, --help              Muestra este mensaje de ayuda y sale.\n";
            return 0;
        } else if (arg == "--explore" || arg == "explore") {
            initial_view = "Explorar";
        } else if (arg == "--updates" || arg == "updates") {
            initial_view = "Actualizaciones";
        } else if (arg.find("--search=") == 0) {
            initial_search = arg.substr(9);
            initial_view = "Explorar";
        } else if (arg == "--search" && i + 1 < argc) {
            initial_search = argv[++i];
            initial_view = "Explorar";
        }
    }

    auto app = std::make_unique<horizon::Application>("org.horizon.horizon-appstore", 1000, 700);
    
    app->set_name("Horizon AppStore");
    app->set_icon_name("system-software-install");

    auto &about = app->about_manager();
    about.set_app_title("Horizon AppStore");
    about.set_app_description("Encuentra e instala aplicaciones en tu sistema.");
    about.set_app_version("1.0.0");
    about.set_app_icon("system-software-install");

    auto window = std::make_unique<horizon::appstore::AppStoreWindow>(initial_view, initial_search);
    app->set_root(std::move(window));
    
    app->run();
    
    return 0;
}
