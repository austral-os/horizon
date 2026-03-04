#include "GlobalMenuMessage.hpp"
#include <horizon/ClientMenu.hpp>
#include <horizon/IpcServer.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/OverlayApplication.hpp>
#include <horizon/Panel.hpp>
#include <horizon/RequestRouter.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

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

        // IPC client for communicating with horizon_menu_manager_d
        ClientMenu client_menu;

        // Root widget
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        // Create a Panel with gradient background
        auto panel = std::make_unique<Panel>();
        panel->set_corner_radius(CornerRadius(0));

        // Create a MenuBar
        auto menubar = std::make_unique<MenuBar>();
        menubar->set_spacing(2);

        menubar->set_spacing(2);

        // Keep a raw pointer to menubar to update it from the timer
        MenuBar *menubar_ptr = menubar.get();

        // Wire up click callback: send menu to daemon for display
        menubar->set_on_menu_click(
            [&client_menu](Menu *menu, int x, int y)
            {
                // y=0 because the menu manager overlay starts right below the
                // top_panel's exclusive zone, so y=0 is already at the panel bottom.
                std::cout << "MenuBar click: " << menu->title() << " at x=" << x << std::endl;
                client_menu.show_menu(menu, x, 0);
            });

        // Setup RequestRouter for top_panel
        MessageManager message_manager; // We don't store messages, but router needs it
        RequestRouter router(message_manager);

        router.register_handler("set_global_menu",
                                [menubar_ptr, &app](const std::string &request_id,
                                                    const nlohmann::json &request,
                                                    MessageManager &mgr) -> nlohmann::json
                                {
                                    std::cout << "Updating global menu..." << std::endl;

                                    // Parse the JSON into a vector of Menu objects
                                    auto new_menus = GlobalMenuMessage::parse(request);

                                    // Update the MenuBar
                                    menubar_ptr->clear_menus();
                                    for (auto &menu : new_menus)
                                    {
                                        menubar_ptr->add_menu(std::move(menu));
                                    }

                                    // Force layout recalculation and repaint
                                    menubar_ptr->invalidate();

                                    nlohmann::json response;
                                    response["status"] = "ok";
                                    response["request_id"] = request_id;
                                    return response;
                                });

        std::mutex queue_mutex;
        std::vector<std::string> pending_messages;

        // Set up IPC Server
        IpcServer server("/tmp/horizon_global_menu.sock",
                         [&](const std::string &msg)
                         {
                             std::lock_guard<std::mutex> lock(queue_mutex);
                             pending_messages.push_back(msg);
                             return "{\"status\": \"received\"}";
                         });

        server.start();

        // Timer to process new messages in the main thread
        app->add_timer(50,
                       [&]()
                       {
                           std::vector<std::string> to_process;
                           {
                               std::lock_guard<std::mutex> lock(queue_mutex);
                               to_process = std::move(pending_messages);
                               pending_messages.clear();
                           }

                           for (const auto &msg : to_process)
                           {
                               router.route(msg);
                           }
                       });

        panel->add_child(std::move(menubar));
        root->add_child(std::move(panel));
        app->set_root(std::move(root));

        std::cout << "Top Panel started (32px)." << std::endl;

        app->run();

        server.stop();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
