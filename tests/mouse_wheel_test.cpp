#include <horizon/Application.hpp>
#include <horizon/Label.hpp>
#include <horizon/Window.hpp>
#include <iostream>

using namespace horizon;

int main()
{
    try
    {
        Application app(400, 300);
        app.set_name("Mouse Wheel Test");

        auto wnd = std::make_unique<Window>("Mouse Wheel Test");
        wnd->set_size(400, 300);

        auto label = std::make_unique<Label>("Scroll over me!");
        label->set_alignment(TextAlignment::Center);

        label->when_mouse_wheel.connect(
            [](MouseWheelEventContext &ev)
            {
                std::cout << "Mouse Wheel Event: dx=" << ev.dx << ", dy=" << ev.dy << " at ("
                          << ev.x << ", " << ev.y << ")" << std::endl;
            });

        wnd->add_child(std::move(label));
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
