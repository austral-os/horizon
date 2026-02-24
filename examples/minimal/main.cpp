#include "horizon/Widget.hpp"
#include <horizon/Application.hpp>
#include <horizon/Button.hpp>
#include <horizon/Window.hpp>
#include <iostream>

int main()
{
    try
    {
        horizon::Application app(800, 600);

        auto wnd = std::make_unique<horizon::Window>("Horizon Application toolkit demo");
        wnd->set_size(800, 600);

        auto container = std::make_unique<horizon::Widget>();
        container->set_margin(10);
        container->set_padding(10);

        auto spacer1 = std::make_unique<horizon::Widget>();
        auto spacer2 = std::make_unique<horizon::Widget>();

        auto btn = std::make_unique<horizon::Button>();
        auto btn2 = std::make_unique<horizon::Button>();

        btn->set_text("Click me");
        btn->set_fixed_size(50);
        btn2->set_text("Click me");
        btn2->set_fixed_size(50);

        container->add_child(std::move(spacer1));
        container->add_child(std::move(btn));
        container->add_child(std::move(btn2));
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
