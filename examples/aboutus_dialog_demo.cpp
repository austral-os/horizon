#include "horizon/dialogs/AboutUsDialog.hpp"
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Window.hpp>
#include <iostream>
#include <memory>

using namespace horizon;

int main(int argc, char **argv)
{
    try
    {
        Application app("horizon.aboutus_demo", 400, 200);
        app.set_name("AboutUs Dialog Demo");

        // Configuración obligatoria del AboutManager
        auto &about = app.about_manager();
        about.set_app_title("AboutUs Demo");
        about.set_app_description("Esta es una demostración del nuevo sistema de diálogos 'Acerca de' de Horizon.");
        about.set_app_version("1.0.0");
        about.set_app_icon("utilities-terminal");
        about.add_app_author("Equipo Horizon", "https://horizon.org", "info@horizon.org");
        about.add_app_translator("Traductor Demo", "https://horizon.org");

        auto wnd = std::make_unique<Window>("Demo de dialogo de AboutUs");
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto btn = std::make_unique<Button<AquaObject>>();
        btn->set_text("Mostrar 'Acerca de'");
        btn->set_fixed_size(50);

        btn->when_click.connect([&app](MouseButtonEventContext &) { app.show_aboutus(); });

        wnd->add_child(std::move(btn));
        app.set_root(std::move(wnd));
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
