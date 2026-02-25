#include "horizon/AquaObject.hpp"
#include "horizon/SolidObject.hpp"
#include "horizon/Widget.hpp"
#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/Window.hpp>
#include <iostream>

using horizon::Application;
using horizon::AquaObject;
using horizon::Button;
using horizon::Widget;
using horizon::WidgetAccentColor;
using horizon::Window;

int main()
{
    try
    {
        Application app(800, 600);

        auto wnd = std::make_unique<Window>("Horizon Application toolkit demo");
        wnd->set_size(800, 600);

        auto container = std::make_unique<Widget>();
        container->set_margin(10);
        container->set_padding(10);

        auto spacer1 = std::make_unique<Widget>();
        auto spacer2 = std::make_unique<Widget>();

        auto btn = std::make_unique<Button<AquaObject>>();
        auto btn2 = std::make_unique<Button<horizon::SolidObject>>();
        auto btn3 = std::make_unique<Button<AquaObject>>();
        auto btn4 = std::make_unique<Button<AquaObject>>();
        auto btn5 = std::make_unique<Button<AquaObject>>();
        auto btn6 = std::make_unique<Button<AquaObject>>();

        btn->set_text("Aceptar");
        btn->set_corner_radius({0, 20, 20, 0});
        btn->set_fixed_size(40);
        btn->set_accent_color(WidgetAccentColor::Primary);

        btn2->set_text("Cancelar");
        // btn2->set_corner_radius({0, 20, 20, 0});
        btn2->set_fixed_size(40);
        btn2->set_accent_color(WidgetAccentColor::Default);

        btn3->set_text("Cancelar");
        btn3->set_fixed_size(40);
        btn3->set_accent_color(WidgetAccentColor::Success);

        btn4->set_text("Cancelar");
        btn4->set_fixed_size(40);
        btn4->set_accent_color(WidgetAccentColor::Error);

        btn5->set_text("Cancelar");
        btn5->set_fixed_size(40);
        btn5->set_accent_color(WidgetAccentColor::Info);

        btn6->set_text("Cancelar");
        btn6->set_fixed_size(40);
        btn6->set_accent_color(WidgetAccentColor::Warning);

        container->add_child(std::move(spacer1));
        container->add_child(std::move(btn));
        container->add_child(std::move(btn2));
        container->add_child(std::move(btn3));
        container->add_child(std::move(btn4));
        container->add_child(std::move(btn5));
        container->add_child(std::move(btn6));
        container->add_child(std::move(spacer2));

        wnd->add_child(std::move(container));

        app.set_root(std::move(wnd));
        app.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
