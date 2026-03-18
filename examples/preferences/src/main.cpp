#include <horizon/WaylandWindow.hpp>
#include "PreferencesWindow.hpp"
#include <memory>

int main()
{
    try
    {
        // 1. Setup the basic Wayland/Horizon context
        horizon::WaylandWindow app("horizon.preferences", 800, 600);
        app.set_name("Preferencias del Sistema");
        app.set_icon_name("preferences-system");
        app.set_show_in_dock(true);

        // 2. Create the main application window
        auto wnd = std::make_unique<horizon::preferences::PreferencesWindow>();
        app.set_root(std::move(wnd));

        // 3. Start the application event loop
        app.run();
    }
    catch (const std::exception &e)
    {
        return 1;
    }

    return 0;
}
