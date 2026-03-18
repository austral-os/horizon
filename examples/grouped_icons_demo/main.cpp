#include <horizon/WaylandWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/GroupedIconsView.hpp>
#include <horizon/Logger.hpp>

using namespace horizon;

int main()
{
    try
    {
        WaylandWindow app("horizon.grouped_icons_demo", 800, 600);
        app.set_name("Grouped Icons Demo");
        app.set_icon_name("preferences-system");

        auto wnd = std::make_unique<Window>("System Preferences");
        wnd->set_size(800, 600);

        auto view = std::make_unique<GroupedIconsView>();
        view->set_position_type(FILL);
        view->set_alternate_colors(Color(1.0f, 1.0f, 1.0f, 1.0f), Color(0.95f, 0.95f, 0.95f, 1.0f));

        // Personal Group
        IconGroup personal;
        personal.title = "Personal";
        personal.items = {
            {"appearance", "Appearance", "preferences-desktop-theme", []() { LOG_INFO << "Appearance clicked"; }},
            {"desktop", "Desktop & Screen Saver", "preferences-desktop-wallpaper", []() { LOG_INFO << "Desktop clicked"; }},
            {"dock", "Dock", "preferences-system-notifications", []() { LOG_INFO << "Dock clicked"; }},
            {"expose", "Exposé & Spaces", "preferences-desktop-screensaver", []() { LOG_INFO << "Exposé clicked"; }},
            {"language", "Language & Text", "preferences-desktop-locale", []() { LOG_INFO << "Language clicked"; }},
            {"security", "Security", "preferences-system-privacy", []() { LOG_INFO << "Security clicked"; }},
            {"spotlight", "Spotlight", "system-search", []() { LOG_INFO << "Spotlight clicked"; }}
        };
        view->add_group(personal);

        // Hardware Group
        IconGroup hardware;
        hardware.title = "Hardware";
        hardware.items = {
            {"cds", "CDs & DVDs", "drive-optical", []() { LOG_INFO << "CDs clicked"; }},
            {"displays", "Displays", "video-display", []() { LOG_INFO << "Displays clicked"; }},
            {"energy", "Energy Saver", "preferences-system-power", []() { LOG_INFO << "Energy clicked"; }},
            {"keyboard", "Keyboard", "input-keyboard", []() { LOG_INFO << "Keyboard clicked"; }},
            {"mouse", "Mouse", "input-mouse", []() { LOG_INFO << "Mouse clicked"; }},
            {"trackpad", "Trackpad", "input-tablet", []() { LOG_INFO << "Trackpad clicked"; }},
            {"print", "Print & Fax", "printer", []() { LOG_INFO << "Print clicked"; }},
            {"sound", "Sound", "audio-card", []() { LOG_INFO << "Sound clicked"; }}
        };
        view->add_group(hardware);

        // Internet & Wireless Group
        IconGroup internet;
        internet.title = "Internet & Wireless";
        internet.items = {
            {"mobileme", "MobileMe", "user-info", []() { LOG_INFO << "MobileMe clicked"; }},
            {"network", "Network", "network-workgroup", []() { LOG_INFO << "Network clicked"; }},
            {"bluetooth", "Bluetooth", "preferences-system-bluetooth", []() { LOG_INFO << "Bluetooth clicked"; }},
            {"sharing", "Sharing", "preferences-system-network", []() { LOG_INFO << "Sharing clicked"; }}
        };
        view->add_group(internet);

        // System Group
        IconGroup system;
        system.title = "System";
        system.items = {
            {"accounts", "Accounts", "user-available", []() { LOG_INFO << "Accounts clicked"; }},
            {"datetime", "Date & Time", "preferences-system-time", []() { LOG_INFO << "Date & Time clicked"; }},
            {"parental", "Parental Controls", "preferences-system-parental-controls", []() { LOG_INFO << "Parental clicked"; }},
            {"software", "Software Update", "system-software-update", []() { LOG_INFO << "Software clicked"; }},
            {"speech", "Speech", "audio-input-microphone", []() { LOG_INFO << "Speech clicked"; }},
            {"startup", "Startup Disk", "drive-harddisk", []() { LOG_INFO << "Startup clicked"; }},
            {"timemachine", "Time Machine", "system-run", []() { LOG_INFO << "Time Machine clicked"; }},
            {"universal", "Universal Access", "preferences-desktop-accessibility", []() { LOG_INFO << "Universal clicked"; }}
        };
        view->add_group(system);

        wnd->add_child(std::move(view));
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
