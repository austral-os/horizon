#include <horizon/Label.hpp>
#include <horizon/OverlayApplication.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <memory>

using namespace horizon;

#include <horizon/Menu.hpp>
#include <horizon/MenuItem.hpp>
#include <horizon/MenuSeparator.hpp>

int main(int argc, char *argv[])
{
    try
    {
        // Create the Menu Manager Daemon
        auto app =
            std::make_unique<OverlayApplication>("horizon_menu_manager_d", 3); // 3 = OVERLAY layer

        // Fullscreen anchor
        app->set_anchor(1 | 2 | 4 | 8); // TOP | BOTTOM | LEFT | RIGHT

        // No exclusive zone - we don't want to move other windows
        app->set_exclusive_zone(0);

        // Enable keyboard interactivity to catch Escape
        app->set_keyboard_interactivity(1); // 1 = EXCLUSIVE

        auto root = std::make_unique<Widget>();
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});

        // --- Complex Menu Structure ---
        auto main_menu = std::make_unique<Menu>();
        main_menu->set_position(50, 50);
        main_menu->set_position_type(FREE);

        // Submenu 1: Settings
        auto settings_menu = std::make_unique<Menu>();
        settings_menu->add_item("Display", "Cmd+D");
        settings_menu->add_item("Sound", "Cmd+S");
        settings_menu->add_item("Network", "Cmd+Option+N");

        // Submenu 2: Preferences
        auto prefs_menu = std::make_unique<Menu>();
        prefs_menu->add_item("General");
        prefs_menu->add_item("Appearance");
        prefs_menu->add_item("Privacy");

        // Submenu 3: Applications (Inside Settings)
        auto apps_menu = std::make_unique<Menu>();
        apps_menu->add_item("App Store");
        apps_menu->add_item("Chrome");
        apps_menu->add_item("Terminator");

        // Nested level: Applications inside Settings
        auto *settings_apps_item = settings_menu->add_item("Developer Apps");
        settings_apps_item->set_submenu(apps_menu.get());

        // Add items to main menu
        main_menu->add_item("About This Horizon");
        main_menu->add_separator();

        auto *settings_item = main_menu->add_item("System Settings");
        settings_item->set_submenu(settings_menu.get());

        auto *prefs_item = main_menu->add_item("Preferences");
        prefs_item->set_submenu(prefs_menu.get());

        main_menu->add_separator();
        main_menu->add_item("Lock Screen", "Cmd+L");
        main_menu->add_item("Log Out...", "Shift+Cmd+Q");

        // Keep submenus alive by adding them as children to root (hidden by default)
        apps_menu->set_visible(false);
        apps_menu->set_position_type(FREE);
        settings_menu->set_visible(false);
        settings_menu->set_position_type(FREE);
        prefs_menu->set_visible(false);
        prefs_menu->set_position_type(FREE);

        // Add everything to root
        root->add_child(std::move(main_menu));
        root->add_child(std::move(settings_menu));
        root->add_child(std::move(prefs_menu));
        root->add_child(std::move(apps_menu));

        app->set_root(std::move(root));

        std::cout << "Horizon Menu Manager Daemon started (Fullscreen Overlay)." << std::endl;
        std::cout << "Root is transparent. Press Escape to quit." << std::endl;

        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
