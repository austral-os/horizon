#include <horizon/IpcClient.hpp>
#include <horizon/IpcServer.hpp>
#include <horizon/Label.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/LayerApplication.hpp>
#include <horizon/RequestRouter.hpp>
#include <horizon/Widget.hpp>
#include <iostream>
#include <linux/input-event-codes.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "MenuMessage.hpp"

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        // Create the Menu Manager Daemon
        auto app =
            std::make_unique<LayerApplication>("horizon_menu_manager_d", 3); // 3 = OVERLAY layer

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

        MessageManager message_manager;
        RequestRouter router(message_manager);

        // Register the "create_menu" handler
        router.register_handler("create_menu",
                                [](const std::string &request_id, const nlohmann::json &request,
                                   MessageManager &mgr) -> nlohmann::json
                                {
                                    if (!request.contains("menu") || !request["menu"].is_object())
                                    {
                                        nlohmann::json err;
                                        err["status"] = "error";
                                        err["request_id"] = request_id;
                                        err["message"] =
                                            "Missing 'menu' object in create_menu request";
                                        return err;
                                    }

                                    auto menu_message =
                                        std::make_unique<MenuMessage>(request["menu"]);
                                    std::string message_id = menu_message->id();

                                    mgr.add_message(std::move(menu_message));

                                    nlohmann::json response;
                                    response["status"] = "ok";
                                    response["request_id"] = request_id;
                                    response["message_id"] = message_id;
                                    return response;
                                });

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
        bool menu_visible = false;
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
                menu_visible = false;
                app->set_visible(false);

                IpcClient global_menu_client("/tmp/horizon_global_menu.sock");
                global_menu_client.send(
                    nlohmann::json{{"type", "menu_daemon_status"}, {"visible", false}}.dump());
            });

        // Hide when escape key is pressed to prevent the daemon from quitting entirely
        root_ptr->when_key_press.connect(
            [&](EventContext &ev)
            {
                if (ev.key == KEY_ESC)
                {
                    std::cout << "Escape pressed, hiding menu manager." << std::endl;
                    for (auto &child : root_ptr->children())
                    {
                        child->set_visible(false);
                    }
                    menu_visible = false;
                    app->set_visible(false);

                    IpcClient global_menu_client("/tmp/horizon_global_menu.sock");
                    global_menu_client.send(
                        nlohmann::json{{"type", "menu_daemon_status"}, {"visible", false}}.dump());
                    ev.stop_propagation = true;
                }
            });

        // Timer to process new messages in the main thread (REPEATING)
        app->add_timer(
            50,
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
                    std::cout << "Processing message in main thread: " << msg << std::endl;
                    auto response = router.route(msg);

                    if (response.contains("message_id"))
                    {
                        std::string id = response["message_id"];
                        Message *message = message_manager.get_message(id);
                        if (message)
                        {
                            auto *menu_msg = static_cast<MenuMessage *>(message);
                            auto menus = menu_msg->release_all_menus();

                            if (!menus.empty())
                            {
                                // Clear old menus before showing new ones
                                if (menu_visible)
                                {
                                    for (auto &child : root_ptr->children())
                                    {
                                        child->set_visible(false);
                                    }
                                    app->invalidate(nullptr);
                                }

                                root_ptr->clear_children();

                                Menu *root_menu_ptr = menus[0].get();

                                // Parse position if provided
                                try
                                {
                                    auto json_msg = nlohmann::json::parse(msg);
                                    int x = json_msg.value("x", 0);
                                    int y = json_msg.value("y", 0);
                                    root_menu_ptr->set_position(x, y);
                                    if (json_msg.contains("monitor"))
                                    {
                                        app->move_to_monitor(json_msg["monitor"]);
                                    }
                                    std::cout << "Positioning menu at (" << x << ", " << y << ")"
                                              << std::endl;
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
                                menu_visible = true;
                                app->set_visible(true);
                                root_ptr->invalidate();

                                IpcClient global_menu_client("/tmp/horizon_global_menu.sock");
                                global_menu_client.send(nlohmann::json{
                                    {"type", "menu_daemon_status"},
                                    {"visible", true}}.dump());
                            }
                        }
                    }
                }
            },
            true);

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
