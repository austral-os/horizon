#include "horizon/dialogs/AboutUsDialog.hpp"
#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/FontDialog.hpp>
#include <iostream>
#include <memory>

using namespace horizon;

int main(int argc, char **argv)
{
    try
    {
        Application app("horizon.aboutus_demo", 400, 200);
        app.set_name("AboutUs Dialog Demo");

        auto wnd = std::make_unique<Window>("Demo de dialogo de AboutUs");
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto btn = std::make_unique<Button<AquaObject>>();
        btn->set_text("About us");
        btn->set_fixed_size(50);

        auto *btn_ptr = btn.get();

        btn->when_click.connect(
            [btn_ptr](MouseButtonEventContext &)
            {
                LOG_INFO << "Abriendo Dialogo...";
                auto dialog = std::make_unique<AboutUsDialog>("About us");

                auto content_about = std::make_unique<Label>();
                content_about->set_text("Este es el contenido");

                auto content_translate = std::make_unique<Label>();
                content_translate->set_text("Este es el contenido de traduccion");

                dialog->set_about_content(std::move(content_about));
                dialog->set_translate_content(std::move(content_translate));

                dialog->show();
                LOG_INFO << "Aboutus cerrado";
            });

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
