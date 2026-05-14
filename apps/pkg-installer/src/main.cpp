#include "InstallerWindow.hpp"
#include <horizon/Application.hpp>

using namespace horizon;

int main(int argc, char** argv) {
    Application app("org.horizon.pkg-installer", 500, 350);
    
    app.set_name("Package Installer");
    app.set_icon_name("system-software-install");

    // Setup About info
    auto &about = app.about_manager();
    about.set_app_title("Package Installer");
    about.set_app_description("Easily install .deb packages by dragging them into your system.");
    about.set_app_version("1.0.0");
    about.set_app_icon("system-software-install");

    auto window = std::make_unique<InstallerWindow>();
    auto* window_ptr = window.get();

    // Handle command line argument
    if (argc > 1) {
        window_ptr->load_deb(argv[1]);
    }

    app.set_root(std::move(window));
    app.run();

    return 0;
}
