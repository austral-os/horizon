#include "ZenitWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>

int main(int argc, char** argv) {
    horizon::Application app("com.horizon.zenit", 1280, 720);
    
    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Zenit");
    about.set_app_description("A high-fidelity video player for Austral OS, powered by horizon-video.");
    about.set_app_version("0.1.0");
    about.set_app_icon("zenit");
    about.set_app_git("https://github.com/austral-os/horizon");
    about.add_app_author("Austral OS Team", "https://github.com/austral-os/horizon");

    auto win = std::make_unique<zenit::ZenitWindow>();
    auto* win_ptr = win.get();
    
    if (argc > 1) {
        win_ptr->open_file(argv[1]);
    }
    
    app.set_root(std::move(win));
    app.run();
    
    return 0;
}
