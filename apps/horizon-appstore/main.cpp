#include "AppStoreWindow.hpp"
#include <horizon/Application.hpp>

int main(int argc, char* argv[]) {
    auto app = std::make_unique<horizon::Application>("org.horizon.horizon-appstore", 1000, 700);
    
    app->set_name("Horizon AppStore");
    app->set_icon_name("system-software-install");

    auto &about = app->about_manager();
    about.set_app_title("Horizon AppStore");
    about.set_app_description("Encuentra e instala aplicaciones en tu sistema.");
    about.set_app_version("1.0.0");
    about.set_app_icon("system-software-install");

    auto window = std::make_unique<horizon::appstore::AppStoreWindow>();
    app->set_root(std::move(window));
    
    app->run();
    
    return 0;
}
