#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/AquaObject.hpp>
#include <core/dialogs/FileDialog/FileDialog.hpp>
#include <horizon/Logger.hpp>
#include <filesystem>
#include <iostream>

using namespace horizon;

int main(int argc, char** argv)
{
    try 
    {
        // 1. Inicializar la aplicación
        Application app("horizon.file_picker_demo", 400, 200);
        app.set_name("File Picker Demo");

        // 2. Crear la ventana principal
        auto wnd = std::make_unique<Window>("Selector de Archivo");
        wnd->set_margin(40);
        wnd->set_spacing(20);
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        // 3. Crear el botón
        auto btn = std::make_unique<Button<AquaObject>>();
        btn->set_text("Abrir archivo...");
        btn->set_fixed_size(50);
        
        // Guardamos el puntero para actualizarlo luego
        auto* btn_ptr = btn.get();

        // 4. Configurar el evento de click
        btn->when_click.connect([&app, btn_ptr](MouseButtonEventContext&) {
            LOG_INFO << "Botón presionado, abriendo FileDialog (modo síncrono)...";
            // Crear el diálogo de archivo en modo "Abrir"
            auto dialog = std::make_unique<FileDialog>(FileDialogMode::Open, "Selecciona un archivo");
            
            // Configurar qué pasa cuando se acepta un archivo
            dialog->when_accepted.connect([btn_ptr](horizon::FileDialogAcceptedContext &ctx) {
                std::filesystem::path p(ctx.selected_path);
                // Cambiar el label del botón al nombre del archivo
                btn_ptr->set_text(p.filename().string());
                LOG_INFO << "Archivo seleccionado: " << ctx.selected_path;
            });

            // Ejecutar el diálogo de forma síncrona en el hilo principal (Patrón ArkFM)
            // Esto bloquea la ventana padre, haciéndola modal.
            dialog->run();
            LOG_INFO << "FileDialog cerrado";
        });

        // 5. Ensamblar y ejecutar
        wnd->add_child(std::move(btn));
        app.set_root(std::move(wnd));
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
