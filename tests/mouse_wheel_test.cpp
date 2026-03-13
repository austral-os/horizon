#include <horizon/Application.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Window.hpp>

using namespace horizon;

int main()
{
    try
    {
        Application app("org.horizon.mouse_wheel_test", 400, 300);
        app.set_name("Mouse Wheel Test");

        auto wnd = std::make_unique<Window>(&app, "Mouse Wheel Test");
        wnd->set_size(400, 300);

        auto label = std::make_unique<Label>("Scroll over me!");
        label->set_alignment(TextAlignment::Center);

        label->when_mouse_wheel.connect(
            [](MouseWheelEventContext &ev)
            {
                LOG_INFO << "Mouse Wheel Event: dx=" << ev.dx << ", dy=" << ev.dy << " at (" << ev.x
                         << ", " << ev.y << ")";
            });

        wnd->add_child(std::move(label));
        app.set_root(std::move(wnd));

        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }
    return 0;
}
