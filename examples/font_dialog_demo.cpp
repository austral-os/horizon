#include <horizon/Application.hpp>
#include <horizon/Window.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/dialogs/FontDialog.hpp>
#include <horizon/Logger.hpp>
#include <filesystem>
#include <iostream>

using namespace horizon;

int main(int argc, char** argv)
{
    try 
    {
        Application app("horizon.font_demo", 400, 200);
        app.set_name("Font Selection Demo");

        auto wnd = std::make_unique<Window>("Demo de Selección de Fuente");
        wnd->set_margin(40);
        wnd->set_spacing(20);
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto btn = std::make_unique<Button<AquaObject>>();
        btn->set_text("Seleccionar fuente...");
        btn->set_fixed_size(50);
        
        auto* btn_ptr = btn.get();

        btn->when_click.connect([btn_ptr](MouseButtonEventContext&) {
            LOG_INFO << "Abriendo FontDialog...";
            auto dialog = std::make_unique<FontDialog>("Seleccionar tipo de letra");
            
            dialog->when_accepted.connect([btn_ptr](horizon::FontDialogAcceptedContext &ctx) {
                LOG_INFO << "Fuente seleccionada: " << ctx.selection.family << " " << ctx.selection.style << " (" << ctx.selection.size << ")";
                btn_ptr->set_text(ctx.selection.family);
            });

            dialog->run();
            LOG_INFO << "FontDialog cerrado";
        });

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
