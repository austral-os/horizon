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
        WaylandWindow app("horizon.aboutus_demo", 400, 200);
        app.set_name("AboutUs Dialog Demo");

        auto wnd = std::make_unique<Window>("Demo de dialogo de AboutUs");
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto btn = std::make_unique<Button<AquaObject>>();
        btn->set_text("About us");
        btn->set_fixed_size(50);

        auto *btn_ptr = btn.get();

        app.set_aboutus_content(
            []()
            {
                auto content_about = std::make_unique<Label>();
                content_about->set_text("Este es el contenido");

                auto content_translate = std::make_unique<Label>();
                content_translate->set_text("Este es el contenido de traduccion");

                auto abus_content = std::make_unique<AboutDialogContent>();
                abus_content->title = "AustralOS";
                abus_content->version = "0.0.1";
                abus_content->icon = "utilities-terminal";
                abus_content->about = std::move(content_about);
                abus_content->translate = std::move(content_translate);

                return abus_content;
            });

        btn->when_click.connect([&app, btn_ptr](MouseButtonEventContext &) { app.show_aboutus(); });

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
