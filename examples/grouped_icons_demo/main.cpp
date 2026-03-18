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

        view->when_item_click.connect([](const GroupedIconItem& item) {
            LOG_INFO << "Item clicked: " << item.label << " (id: " << item.id << ")";
        });

        view->when_item_dbl_click.connect([](const GroupedIconItem& item) {
            LOG_INFO << "Item double-clicked: " << item.label;
        });

        // Personal Group
        IconGroup personal;
        personal.title = "Personal";
        personal.items = {
            {"appearance", "Appearance", "preferences-desktop-theme"},
            {"desktop", "Desktop & Screen Saver", "preferences-desktop-wallpaper"},
            {"dock", "Dock", "preferences-system-notifications"},
            {"expose", "Exposé & Spaces", "preferences-desktop-screensaver"},
            {"language", "Language & Text", "preferences-desktop-locale"},
            {"security", "Security", "preferences-system-privacy"},
            {"spotlight", "Spotlight", "system-search"}
        };
        view->add_group(personal);

        // Hardware Group
        IconGroup hardware;
        hardware.title = "Hardware";
        hardware.items = {
            {"cds", "CDs & DVDs", "drive-optical"},
            {"displays", "Displays", "video-display"},
            {"energy", "Energy Saver", "preferences-system-power"},
            {"keyboard", "Keyboard", "input-keyboard"},
            {"mouse", "Mouse", "input-mouse"},
            {"trackpad", "Trackpad", "input-tablet"},
            {"print", "Print & Fax", "printer"},
            {"sound", "Sound", "audio-card"}
        };
        view->add_group(hardware);

        // Internet & Wireless Group
        IconGroup internet;
        internet.title = "Internet & Wireless";
        internet.items = {
            {"mobileme", "MobileMe", "user-info"},
            {"network", "Network", "network-workgroup"},
            {"bluetooth", "Bluetooth", "preferences-system-bluetooth"},
            {"sharing", "Sharing", "preferences-system-network"}
        };
        view->add_group(internet);

        // System Group
        IconGroup system;
        system.title = "System";
        system.items = {
            {"accounts", "Accounts", "user-available"},
            {"datetime", "Date & Time", "preferences-system-time"},
            {"parental", "Parental Controls", "preferences-system-parental-controls"},
            {"software", "Software Update", "system-software-update"},
            {"speech", "Speech", "audio-input-microphone"},
            {"startup", "Startup Disk", "drive-harddisk"},
            {"timemachine", "Time Machine", "system-run"},
            {"universal", "Universal Access", "preferences-desktop-accessibility"}
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
