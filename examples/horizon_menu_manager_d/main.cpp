#include <horizon/Label.hpp>
#include <horizon/OverlayApplication.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

#include "DialogManager.hpp"
#include "IpcServer.hpp"
#include "RequestRouter.hpp"

using namespace horizon;

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

        // Use a raw pointer for callbacks since ownership of 'root' will be moved
        Widget *root_ptr = root.get();

        DialogManager dialog_manager;
        RequestRouter router(dialog_manager);

        std::mutex queue_mutex;
        std::vector<std::string> pending_messages;

        // Set up IPC Server
        IpcServer server("/tmp/horizon_menu.sock",
                         [&](const std::string &msg)
                         {
                             std::lock_guard<std::mutex> lock(queue_mutex);
                             pending_messages.push_back(msg);

                             // Return a placeholder response. Real response will be asynchronous
                             // but for this protocol, "ok" is enough to acknowledge receipt.
                             return "{\"status\": \"received\"}";
                         });

        server.start();

        // Initially hidden
        app->set_visible(false);

        // Hide when clicking the background (root widget)
        root_ptr->add_on_mouse_press(
            [&](int btn)
            {
                std::cout << "Click on background, hiding menu manager." << std::endl;
                for (auto &child : root_ptr->children())
                {
                    child->set_visible(false);
                }
                app->set_visible(false);
            });

        // Timer to process new dialogs in the main thread
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
                               std::cout << "Processing message in main thread: " << msg
                                         << std::endl;
                               auto response = router.route(msg);

                               if (response.contains("dialog_id"))
                               {
                                   std::string id = response["dialog_id"];
                                   Dialog *dialog = dialog_manager.get_dialog(id);
                                   if (dialog)
                                   {
                                       auto *menu_dialog = static_cast<MenuDialog *>(dialog);
                                       auto menus = menu_dialog->release_all_menus();

                                       if (!menus.empty())
                                       {
                                           // Clear old menus before showing new ones
                                           root_ptr->clear_children();

                                           Menu *root_menu_ptr = menus[0].get();

                                           // Parse position if provided
                                           try
                                           {
                                               auto json_msg = nlohmann::json::parse(msg);
                                               int x = json_msg.value("x", 0);
                                               int y = json_msg.value("y", 0);
                                               root_menu_ptr->set_position(x, y);
                                               std::cout << "Positioning menu at (" << x << ", "
                                                         << y << ")" << std::endl;
                                           }
                                           catch (...)
                                           {
                                               root_menu_ptr->set_position(0, 0);
                                           }

                                           // Add and show all menus (root and submenus)
                                           for (auto &menu : menus)
                                           {
                                               menu->set_visible(false); // Submenus start hidden
                                               root_ptr->add_child(std::move(menu));
                                           }

                                           // Ensure layout is calculated before showing
                                           root_menu_ptr->calculate_layout();
                                           root_menu_ptr->set_visible(true);
                                           app->set_visible(true);
                                           root_ptr->invalidate();
                                       }
                                   }
                               }
                           }
                       });

        app->set_root(std::move(root));

        std::cout << "Horizon Menu Manager Daemon started." << std::endl;
        std::cout << "IPC Server: /tmp/horizon_menu.sock" << std::endl;
        std::cout << "Application is hidden and waiting for requests." << std::endl;

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
