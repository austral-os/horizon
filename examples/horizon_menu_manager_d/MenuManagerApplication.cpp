#include "MenuManagerApplication.hpp"
#include "MenuMessage.hpp"
#include <horizon/Logger.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <linux/input-event-codes.h>
#include <nlohmann/json.hpp>

namespace horizon
{
    MenuManagerApplication::MenuManagerApplication()
        : Application("org.horizon.menu_manager_d", 800, 600, true, true)
    {
        m_window = create_layer_window("horizon_menu_manager_d", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY);

        m_window->set_name("Horizon Menu Manager");
        m_window->set_show_in_dock(false);

        setup_window();
        setup_ipc();

        auto root = std::make_unique<Widget>();
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});
        m_root_ptr = root.get();

        setup_event_handlers();

        m_window->set_root(std::move(root));
        m_window->set_visible(false);

        LOG_INFO << "Horizon Menu Manager Daemon initialized.";
    }

    MenuManagerApplication::~MenuManagerApplication() = default;

    void MenuManagerApplication::setup_window()
    {
        m_window->set_anchor(
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        m_window->set_exclusive_zone(-1);
        m_window->set_keyboard_interactivity(
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    }

    void MenuManagerApplication::setup_ipc()
    {
        m_router = std::make_unique<RequestRouter>(m_message_manager);

        m_router->register_handler(
            "create_menu",
            [this](const std::string &request_id, const nlohmann::json &request,
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

                auto on_action = [this]() { hide_daemon(); };

                std::string requester_id = request.value("requester_id", "");
                int requester_pid = request.value("requester_pid", -1);
                auto menu_message = std::make_unique<MenuMessage>(request["menu"], requester_id,
                                                                  requester_pid, on_action);
                std::string message_id = menu_message->id();

                mgr.add_message(std::move(menu_message));

                nlohmann::json response;
                response["status"] = "ok";
                response["request_id"] = request_id;
                response["message_id"] = message_id;
                return response;
            });

        m_ipc_client = std::make_unique<IpcClient>("/tmp/horizon_session.sock");
        m_ipc_client->subscribe("{\"type\": \"subscribe\"}",
                                [this](const std::string &msg)
                                {
                                    try
                                    {
                                        auto j = nlohmann::json::parse(msg);
                                        if (j.value("receiver_id", "") == "horizon_menu_manager_d")
                                        {
                                            std::lock_guard<std::mutex> lock(m_queue_mutex);
                                            m_pending_messages.push_back(msg);
                                        }
                                    }
                                    catch (...)
                                    {
                                    }
                                });

        m_window->add_timer(50, [this]() { process_messages(); }, true);
    }

    void MenuManagerApplication::setup_event_handlers()
    {
        m_root_ptr->when_mouse_press.connect(
            [this](EventContext &)
            {
                LOG_INFO << "Click on background, hiding menu manager.";
                hide_daemon();
            });

        m_root_ptr->when_key_press.connect(
            [this](KeyEventContext &ev)
            {
                if (ev.key == KEY_ESC)
                {
                    LOG_INFO << "Escape pressed, hiding menu manager.";
                    hide_daemon();
                    ev.stop_propagation = true;
                }
            });
    }

    void MenuManagerApplication::process_messages()
    {
        std::vector<std::string> to_process;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (m_pending_messages.empty())
                return;
            to_process = std::move(m_pending_messages);
            m_pending_messages.clear();
        }

        for (const auto &msg : to_process)
        {
            LOG_INFO << "Processing message in main thread: " << msg;
            auto response = m_router->route(msg);

            if (response.contains("message_id"))
            {
                std::string id = response["message_id"];
                Message *message = m_message_manager.get_message(id);
                if (message)
                {
                    auto *menu_msg = static_cast<MenuMessage *>(message);
                    auto menus = menu_msg->release_all_menus();

                    if (!menus.empty())
                    {
                        if (m_menu_visible)
                        {
                            for (auto &child : m_root_ptr->children())
                            {
                                child->set_visible(false);
                            }
                            m_window->invalidate();
                        }

                        m_root_ptr->clear_children();
                        Menu *root_menu_ptr = menus[0].get();

                        try
                        {
                            auto json_msg = nlohmann::json::parse(msg);
                            int x = json_msg.value("x", 0);
                            int y = json_msg.value("y", 0);

                            if (x == -1 && y == -1)
                            {
                                x = m_window->w_surface()->pointer_x();
                                y = m_window->w_surface()->pointer_y();
                            }

                            root_menu_ptr->set_position(x, y);
                            if (json_msg.contains("monitor"))
                            {
                                m_window->move_to_monitor(json_msg["monitor"]);
                            }

                            // Adjust position if it goes out of bounds
                            root_menu_ptr->calculate_layout();
                            int menu_w = root_menu_ptr->width();
                            int menu_h = root_menu_ptr->height();
                            int screen_w = m_window->width();
                            int screen_h = m_window->height();

                            int adjusted_x = std::max(0, std::min(x, screen_w - menu_w));
                            int adjusted_y = std::max(0, std::min(y, screen_h - menu_h));

                            if (adjusted_x != x || adjusted_y != y)
                            {
                                LOG_INFO << "Menu position adjusted from (" << x << ", " << y
                                         << ") to (" << adjusted_x << ", " << adjusted_y
                                         << ") to stay within screen boundaries.";
                                root_menu_ptr->set_position(adjusted_x, adjusted_y);
                                root_menu_ptr->calculate_layout();
                            }
                        }
                        catch (...)
                        {
                            root_menu_ptr->set_position(0, 0);
                        }

                        for (auto &menu : menus)
                        {
                            menu->set_visible(false);
                            m_root_ptr->add_child(std::move(menu));
                        }

                        root_menu_ptr->calculate_layout();
                        root_menu_ptr->set_visible(true);
                        m_menu_visible = true;
                        m_window->set_visible(true);
                        m_root_ptr->invalidate();

                        IpcClient global_menu_client("/tmp/horizon_session.sock");
                        global_menu_client.send(nlohmann::json{{"type", "menu_daemon_status"},
                                                               {"visible", true},
                                                               {"receiver_id", "top_panel"}}
                                                    .dump());
                    }
                }
            }
        }
    }

    void MenuManagerApplication::hide_daemon()
    {
        for (auto &child : m_root_ptr->children())
        {
            child->set_visible(false);
        }
        m_menu_visible = false;
        m_window->set_visible(false);

        IpcClient global_menu_client("/tmp/horizon_session.sock");
        global_menu_client.send(nlohmann::json{
            {"type", "menu_daemon_status"}, {"visible", false}, {"receiver_id", "top_panel"}}
                                    .dump());
    }
} // namespace horizon
