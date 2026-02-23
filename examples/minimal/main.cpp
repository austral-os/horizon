#include "horizon/Window.hpp"
#include <horizon/Application.hpp>
#include <iostream>

int main()
{
    try
    {
        horizon::Application app(800, 600);

        auto wnd = std::make_unique<horizon::Window>("Horizon Application toolkit demo");
        wnd->set_size(800, 600);

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
