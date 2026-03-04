#include "horizon/Menu.hpp"
#include "horizon/MenuBar.hpp"
#include "horizon/OverlayApplication.hpp"
#include "horizon/Panel.hpp"
#include "horizon/Widget.hpp"
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <iostream>
#include <memory>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        // Create an overlay application
        auto app = std::make_unique<OverlayApplication>("top_panel",
                                                        3); // 3 = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY

        // Anchor to TOP, LEFT, RIGHT to occupy the top of the screen
        app->set_anchor(1 | 4 | 8); // TOP | LEFT | RIGHT

        // Set height to 32, width to 0 (ignored by anchors)
        app->set_size(0, 32);

        // Set exclusive zone to 32 so other windows don't overlap
        app->set_exclusive_zone(32);

        // Enable keyboard interactivity to catch the Escape key
        app->set_keyboard_interactivity(
            1); // 1 = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE

        // Root widget
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        // Create a Panel with gradient background
        auto panel = std::make_unique<Panel>();
        panel->set_corner_radius(CornerRadius(0));

        // Create a MenuBar
        auto menubar = std::make_unique<MenuBar>();
        menubar->set_spacing(2);

        // File menu
        auto fileMenu = std::make_unique<Menu>();
        fileMenu->set_title("File");
        fileMenu->add_item("New", "⌃N");
        fileMenu->add_item("Open", "⌃O");
        fileMenu->add_item("Save", "⌃S");
        fileMenu->add_item("Save As…", "⇧⌃S");
        fileMenu->add_separator();
        fileMenu->add_item("Exit", "⌃Q");

        // Edit menu
        auto editMenu = std::make_unique<Menu>();
        editMenu->set_title("Edit");
        editMenu->add_item("Undo", "⌃Z");
        editMenu->add_item("Redo", "⇧⌃Z");
        editMenu->add_separator();
        editMenu->add_item("Cut", "⌃X");
        editMenu->add_item("Copy", "⌃C");
        editMenu->add_item("Paste", "⌃V");

        // View menu
        auto viewMenu = std::make_unique<Menu>();
        viewMenu->set_title("View");
        viewMenu->add_item("Zoom In", "⌃+");
        viewMenu->add_item("Zoom Out", "⌃-");
        viewMenu->add_separator();
        viewMenu->add_item("Full Screen", "F11");

        // Help menu
        auto helpMenu = std::make_unique<Menu>();
        helpMenu->set_title("Help");
        helpMenu->add_item("About Austral OS");
        helpMenu->add_item("Documentation");

        menubar->add_menu(std::move(fileMenu));
        menubar->add_menu(std::move(editMenu));
        menubar->add_menu(std::move(viewMenu));
        menubar->add_menu(std::move(helpMenu));

        panel->add_child(std::move(menubar));
        root->add_child(std::move(panel));
        app->set_root(std::move(root));

        std::cout << "Top Panel started (32px)." << std::endl;

        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
