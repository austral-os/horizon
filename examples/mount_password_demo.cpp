#include <horizon/Application.hpp>
#include <horizon/storage/MountPasswordDialog.hpp>
#include <horizon/Logger.hpp>
#include <thread>

using namespace horizon;
using namespace horizon::storage;

int main(int argc, char** argv)
{
    Application app("horizon.storage.mount_demo", 700, 400);
    app.about_manager().set_app_title("Mount Demo");
    app.about_manager().set_app_description("Demo for mount password dialog with loading/error states");
    app.about_manager().set_app_version("0.2.0");
    app.about_manager().set_app_icon("network-server");
    
    MountPasswordDialog dialog("ftp://test.rebex.net/");
    
    // Capturamos el puntero para poder manipular el diálogo desde el callback
    auto* dialog_ptr = &dialog;
    auto* app_ptr = &app;

    dialog.when_accepted.connect([dialog_ptr](MountPasswordEvent& ev) {
        LOG_INFO << "Demo: Intento de conexión con usuario: " << ev.credentials.username;
        
        // Simulamos un retraso de red de 2 segundos
        dialog_ptr->add_timer(2000, [dialog_ptr]() {
            // Simulamos un error para que Horacio vea la interfaz de error
            dialog_ptr->show_error("Error de autenticación: El nombre de usuario o la contraseña son incorrectos.");
            LOG_INFO << "Demo: Mostrando error simulado.";
        }, false);
    });
    
    dialog.run();
    
    return 0;
}
