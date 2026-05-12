#include <horizon/Application.hpp>
#include <horizon/storage/MountPasswordDialog.hpp>
#include <horizon/Logger.hpp>

using namespace horizon;
using namespace horizon::storage;

int main(int argc, char** argv)
{
    // Application setup (minimal)
    Application app("horizon.storage.mount_demo", 520, 360);
    app.about_manager().set_app_title("Mount Demo");
    app.about_manager().set_app_description("Demo for mount password dialog");
    app.about_manager().set_app_version("0.1.0");
    app.about_manager().set_app_icon("network-server");
    
    // Create the dialog
    MountPasswordDialog dialog("ftp://test.rebex.net/");
    
    dialog.when_accepted.connect([](MountPasswordEvent& ev) {
        LOG_INFO << "Demo: Credenciales aceptadas";
        LOG_INFO << "  Invitado: " << (ev.credentials.is_guest ? "Sí" : "No");
        LOG_INFO << "  Usuario: " << ev.credentials.username;
        LOG_INFO << "  Password: " << std::string(ev.credentials.password.length(), '*');
        LOG_INFO << "  Recordar: " << (ev.credentials.remember ? "Sí" : "No");
    });
    
    // Run the dialog event loop
    dialog.run();
    
    return 0;
}
