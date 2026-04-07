#include "PreferencesWindow.hpp"
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <memory>

int main(int argc, char *argv[])
{
    try
    {
        // 0. Initialize Logger
        horizon::Logger::instance().init("preferences");

        // 1. Setup the basic Wayland/Horizon context
        horizon::WaylandWindow app("horizon.preferences", 800, 650);
        app.set_name("Preferencias del Sistema");
        app.set_icon_name("preferences-system");
        app.set_show_in_dock(true);

        // Parse initial section from arguments
        std::string initial_section = "home";
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg.find("--") == 0)
            {
                std::string section = arg.substr(2);
                if (section == "wifi")
                    section = "wi-fi";

                initial_section = section;
            }
        }

        // 2. Create the main application window
        auto wnd = std::make_unique<horizon::preferences::PreferencesWindow>(initial_section);
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
