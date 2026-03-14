#include <horizon/IpcClient.hpp>

#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/RequestRouter.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Widget.hpp>
#include <linux/input-event-codes.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "MenuMessage.hpp"
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>

using namespace horizon;

int main(int argc, char *argv[])
{
    try
    {
        // Create the Menu Manager Daemon
        auto app = std::make_unique<WaylandLayerWindow>(
            "horizon_menu_manager_d", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY); // 3 = OVERLAY layer

        app->set_name("Horizon Menu Manager");
        app->set_show_in_dock(false);

        // Fullscreen anchor
        // app->set_anchor(1 | 2 | 4 | 8); // TOP | BOTTOM | LEFT | RIGHT
        app->set_anchor(ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

        // No exclusive zone - we don't want to move other windows
        app->set_exclusive_zone(-1);

        // Enable keyboard interactivity to catch Escape
        app->set_keyboard_interactivity(
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE); // 1 = EXCLUSIVE

        auto root = std::make_unique<Widget>();
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});

        // Use a raw pointer for callbacks since ownership of 'root' will be moved
        Widget *root_ptr = root.get();

        MessageManager message_manager;
        RequestRouter router(message_manager);

        // State for managing global menu locking
        bool menu_visible = false;

        // Register the "create_menu" handler
        router.register_handler(
            "create_menu",
            [&app, &menu_visible, root_ptr](const std::string &request_id,
                                            const nlohmann::json &request,
                                            MessageManager &mgr) -> nlohmann::json
            {
                if (!request.contains("menu") || !request["menu"].is_object())
                {
                    nlohmann::json err;
                    err["status"] = "error";
                    err["request_id"] = request_id;
                    err["message"] = "Missing 'menu' object in create_menu request";
                    return err;
                }

                // Define a reusable lambda to hide the daemon and notify others
                auto hide_daemon = [&app, &menu_visible, root_ptr]()
                {
                    LOG_INFO << "[MENU MANAGER] Hiding daemon via callback.";
                    for (auto &child : root_ptr->children())
                    {
                        child->set_visible(false);
                    }
                    menu_visible = false;
                    app->set_visible(false);

                    IpcClient global_menu_client("/tmp/horizon_session.sock");
                    global_menu_client.send(nlohmann::json{{"type", "menu_daemon_status"},
                                                           {"visible", false},
                                                           {"receiver_id", "top_panel"}}
                                                .dump());
                };

                std::string requester_id = request.value("requester_id", "");
                int requester_pid = request.value("requester_pid", -1);
                auto menu_message = std::make_unique<MenuMessage>(request["menu"], requester_id,
                                                                  requester_pid, hide_daemon);
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

        // Set up IPC Client
        auto ipc_client = std::make_unique<IpcClient>("/tmp/horizon_session.sock");
        ipc_client->subscribe("{\"type\": \"subscribe\"}",
                              [&queue_mutex, &pending_messages](const std::string &msg)
                              {
                                  try
                                  {
                                      auto j = nlohmann::json::parse(msg);
                                      if (j.value("receiver_id", "") == "horizon_menu_manager_d")
                                      {
                                          std::lock_guard<std::mutex> lock(queue_mutex);
                                          pending_messages.push_back(msg);
                                      }
                                  }
                                  catch (...)
                                  {
                                  }
                              });

        app->set_visible(false);

        // Hide when clicking the background (root widget)
        root_ptr->add_on_mouse_press(
            [&](int btn)
            {
                LOG_INFO << "Click on background at unknown coordinates (button " << btn
                         << "), hiding menu manager.";
                for (auto &child : root_ptr->children())
                {
                    child->set_visible(false);
                }
                menu_visible = false;
                app->set_visible(false);

                IpcClient global_menu_client("/tmp/horizon_session.sock");
                global_menu_client.send(nlohmann::json{{"type", "menu_daemon_status"},
                                                       {"visible", false},
                                                       {"receiver_id", "top_panel"}}
                                            .dump());
            });

        // Hide when escape key is pressed to prevent the daemon from quitting entirely
        root_ptr->when_key_press.connect(
            [&](KeyEventContext &ev)
            {
                if (ev.key == KEY_ESC)
                {
                    LOG_INFO << "Escape pressed, hiding menu manager.";
                    for (auto &child : root_ptr->children())
                    {
                        child->set_visible(false);
                    }
                    menu_visible = false;
                    app->set_visible(false);

                    IpcClient global_menu_client("/tmp/horizon_session.sock");
                    global_menu_client.send(nlohmann::json{{"type", "menu_daemon_status"},
                                                           {"visible", false},
                                                           {"receiver_id", "top_panel"}}
                                                .dump());
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
                    LOG_INFO << "Processing message in main thread: " << msg;
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
                                    LOG_INFO << "Positioning menu at (" << x << ", " << y << ")";
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

                                IpcClient global_menu_client("/tmp/horizon_session.sock");
                                global_menu_client.send(
                                    nlohmann::json{{"type", "menu_daemon_status"},
                                                   {"visible", true},
                                                   {"receiver_id", "top_panel"}}
                                        .dump());
                            }
                        }
                    }
                }
            },
            true);

        app->set_root(std::move(root));

        LOG_INFO << "Horizon Menu Manager Daemon started.";
        LOG_INFO << "IPC Target: /tmp/horizon_session.sock";
        LOG_INFO << "Application is hidden and waiting for requests.";

        app->run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }

    return 0;
}
