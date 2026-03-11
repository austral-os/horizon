#include "GlobalMenuMessage.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/DesktopEntry.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/LayerApplication.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/MenuBar.hpp>
#include <horizon/MessageManager.hpp>
#include <horizon/Panel.hpp>
#include <horizon/RequestRouter.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <memory>
#include <mutex>
#include <vector>

using namespace horizon;

const int PANEL_HEIGHT = 32;

int main(int argc, char *argv[])
{
    try
    {
        // Create an overlay application
        auto app = std::make_unique<LayerApplication>("top_panel",
                                                      2); // 2 = ZWLR_LAYER_SHELL_V1_LAYER_TOP
        app->set_name("Top Panel");
        app->set_show_in_dock(false);

        // Add custom search path for .desktop files
        DesktopEntry::add_search_path(
            "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/");

        // Anchor to TOP, LEFT, RIGHT to occupy the top of the screen
        app->set_anchor(1 | 4 | 8); // TOP | LEFT | RIGHT

        // Set height to 32, width to 0 (ignored by anchors)
        app->set_size(0, PANEL_HEIGHT);

        // Set exclusive zone to 32 so other windows don't overlap
        app->set_exclusive_zone(PANEL_HEIGHT);

        // Disable keyboard interactivity to prevent focus stealing
        app->set_keyboard_interactivity(0); // 0 = NONE

        app->set_show_in_dock(false);
        app->set_visible(true); // Enable input region

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

        // Keep a raw pointer to menubar to update it from the timer
        MenuBar *menubar_ptr = menubar.get();

        auto create_system_menu = []()
        {
            auto menu = std::make_unique<Menu>();
            menu->set_title("");
            menu->set_icon_name("start-here-symbolic"); // Tentative icon name
            if (menu->icon_name().empty() ||
                IconThemeLookup::find_icon(menu->icon_name(), 18).empty())
            {
                menu->set_icon_name("start-here"); // Fallback
            }

            auto *about_mnu = menu->add_item("About This System");
            about_mnu->set_id("run_aboutus");

            menu->add_separator();
            auto *terminal = menu->add_item("Terminal");
            terminal->set_id("run_terminal");
            menu->add_item("System Settings...");
            menu->add_item("App Store...");
            menu->add_separator();
            menu->add_item("Recent Items");
            menu->add_separator();
            auto *force_quit = menu->add_item("Force Quit...");
            force_quit->set_id("force_quit");
            menu->add_separator();
            menu->add_item("Sleep");
            menu->add_item("Restart...");
            menu->add_item("Shut Down...");
            menu->add_separator();
            menu->add_item("Lock Screen");
            auto *logout = menu->add_item("Log Out...");
            logout->set_id("run_logout");

            return menu;
        };

        // State for managing global menu locking (preventing blink on focus loss)
        bool menu_daemon_visible = false;
        bool has_cached_menu_request = false;
        nlohmann::json cached_menu_request;
        size_t apply_cache_timer_id = 0;
        int current_owner_pid = -1;
        size_t clear_menu_timer_id = 0;

        auto apply_global_menu_fn = [menubar_ptr, create_system_menu](const nlohmann::json &request)
        {
            auto new_menus = GlobalMenuMessage::parse(request);
            menubar_ptr->clear_menus();

            // Always add system menu first
            menubar_ptr->add_menu(create_system_menu());

            for (auto &menu : new_menus)
            {
                menubar_ptr->add_menu(std::move(menu));
            }
            menubar_ptr->invalidate();
        };

        // Initialize with system menu
        apply_global_menu_fn(nlohmann::json::object());

        // Wire up click callback: send menu to daemon for display
        menubar->set_on_menu_click(
            [&client_menu, &menu_daemon_visible, &current_owner_pid](Menu *menu, int x, int y)
            {
                // Synchronously assume daemon is visible to prevent race conditions
                menu_daemon_visible = true;

                // y=0 because the menu manager overlay starts right below the
                // top_panel's exclusive zone, so y=0 is already at the panel bottom.
                LOG_INFO << "MenuBar click: " << menu->title() << " at x=" << x
                         << " (Owner PID: " << current_owner_pid << ")";
                client_menu.show_menu(menu, x, PANEL_HEIGHT, -1, "top_panel", current_owner_pid);
            });

        // Setup RequestRouter for top_panel
        MessageManager message_manager; // We don't store messages, but router needs it
        RequestRouter router(message_manager);

        router.register_handler(
            "set_global_menu",
            [&menu_daemon_visible, &has_cached_menu_request, &cached_menu_request,
             &apply_cache_timer_id, &current_owner_pid, &clear_menu_timer_id, apply_global_menu_fn,
             &app, menubar_ptr](const std::string &request_id, const nlohmann::json &request,
                                MessageManager &mgr) -> nlohmann::json
            {
                int request_pid = request.value("pid", -1);
                auto menus_json = request.value("menus", nlohmann::json::array());
                bool is_empty = menus_json.empty();

                LOG_INFO << "[TOP PANEL] [IPC THREAD] Received set_global_menu from PID "
                         << request_pid << " (menus count: " << menus_json.size() << ")";

                // Move UI modifications to the main thread
                app->post_task(
                    [menubar_ptr, request, request_pid, is_empty, &current_owner_pid,
                     &clear_menu_timer_id, &app, apply_global_menu_fn]()
                    {
                        LOG_INFO << "[TOP PANEL] [MAIN THREAD] Processing request from PID "
                                 << request_pid;

                        if (!is_empty)
                        {
                            if (clear_menu_timer_id)
                            {
                                LOG_INFO
                                    << "[TOP PANEL] [MAIN THREAD] Cancelling pending clear timer.";
                                app->stop_timer(clear_menu_timer_id);
                                clear_menu_timer_id = 0;
                            }

                            current_owner_pid = request_pid;
                            LOG_INFO << "[TOP PANEL] [MAIN THREAD] Applying menu for PID "
                                     << current_owner_pid;
                            apply_global_menu_fn(request);
                        }
                        else
                        {
                            if (current_owner_pid == -1 || request_pid == current_owner_pid)
                            {
                                if (clear_menu_timer_id)
                                {
                                    app->stop_timer(clear_menu_timer_id);
                                }

                                LOG_INFO << "[TOP PANEL] [MAIN THREAD] Scheduling clear for PID "
                                         << request_pid << " in 100ms.";
                                clear_menu_timer_id = app->add_timer(
                                    100,
                                    [apply_global_menu_fn, &current_owner_pid,
                                     &clear_menu_timer_id]()
                                    {
                                        LOG_INFO << "[TOP PANEL] [TIMER] Resetting global menu "
                                                    "to default (Current owner PID "
                                                 << current_owner_pid << ").";
                                        apply_global_menu_fn(nlohmann::json::object());
                                        current_owner_pid = -1;
                                        clear_menu_timer_id = 0;
                                    });
                            }
                            else
                            {
                                LOG_INFO << "[TOP PANEL] [MAIN THREAD] Ignoring clear from PID "
                                         << request_pid << " (Owner is " << current_owner_pid
                                         << ")";
                            }
                        }
                    });

                nlohmann::json response;
                response["status"] = "ok";
                response["request_id"] = request_id;
                return response;
            });

        router.register_handler(
            "menu_item_clicked",
            [&app, &current_owner_pid](const std::string &request_id, const nlohmann::json &request,
                                       MessageManager &mgr) -> nlohmann::json
            {
                std::string item_id = request.value("id", "");
                LOG_INFO << "[TOP PANEL] Menu item clicked: " << item_id;

                if (item_id == "run_terminal")
                {
                    LOG_INFO << "[TOP PANEL] Requesting to run terminal...";
                    app->send_remote_signal(-1, "run_app", "konsole");
                }
                else if (item_id == "run_aboutus")
                {
                    LOG_INFO << "[TOP PANEL] Requesting to run aboutus...";
                    ApplicationLauncher::launch("aboutus");
                }
                else if (item_id == "run_logout")
                {
                    LOG_INFO << "[TOP PANEL] Requesting Logout...";
                    app->send_remote_signal(-1, "logout");
                }
                else if (item_id == "force_quit")
                {
                    if (current_owner_pid != -1)
                    {
                        LOG_INFO << "[TOP PANEL] Requesting Force Quit for PID "
                                 << current_owner_pid;
                        app->send_remote_signal(current_owner_pid, "kill");
                    }
                    else
                    {
                        LOG_INFO << "[TOP PANEL] Force Quit requested but no app is focused.";
                    }
                }

                nlohmann::json response;
                response["status"] = "ok";
                response["request_id"] = request_id;
                return response;
            });

        router.register_handler(
            "menu_daemon_status",
            [&menu_daemon_visible, &has_cached_menu_request, &cached_menu_request,
             &apply_cache_timer_id, apply_global_menu_fn, &app,
             menubar_ptr](const std::string &request_id, const nlohmann::json &request,
                          MessageManager &mgr) -> nlohmann::json
            {
                menu_daemon_visible = request.value("visible", false);
                LOG_INFO << "Menu daemon status updated: " << menu_daemon_visible;

                if (!menu_daemon_visible && has_cached_menu_request && !apply_cache_timer_id)
                {
                    // Wait 150ms to allow the previous focused app to regain focus
                    // and send a fresh global menu (overwriting the cache).
                    apply_cache_timer_id = app->add_timer(
                        150,
                        [apply_global_menu_fn, &apply_cache_timer_id, &has_cached_menu_request,
                         &cached_menu_request, menubar_ptr]()
                        {
                            if (has_cached_menu_request)
                            {
                                LOG_INFO << "Applying cached menu request after unlock timer.";
                                apply_global_menu_fn(cached_menu_request);
                                has_cached_menu_request = false;
                            }
                            menubar_ptr->set_menu_open(false);
                            apply_cache_timer_id = 0;
                        });
                }
                else
                {
                    menubar_ptr->set_menu_open(menu_daemon_visible);
                }

                nlohmann::json response;
                response["status"] = "ok";
                response["request_id"] = request_id;
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
                                      if (j.value("receiver_id", "") == "top_panel")
                                      {
                                          std::lock_guard<std::mutex> lock(queue_mutex);
                                          pending_messages.push_back(msg);
                                      }
                                  }
                                  catch (...)
                                  {
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
                    if (pending_messages.empty())
                        return;

                    LOG_INFO << "[TOP PANEL] [TIMER] Picking up " << pending_messages.size()
                             << " messages from queue.";
                    to_process = std::move(pending_messages);
                    pending_messages.clear();
                }

                for (const auto &msg : to_process)
                {
                    LOG_INFO << "[TOP PANEL] [TIMER] Routing message...";
                    router.route(msg);
                }
            },
            true);

        panel->add_child(std::move(menubar));
        root->add_child(std::move(panel));
        app->set_root(std::move(root));

        LOG_INFO << "Top Panel started (32px).";

        app->run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error: " << e.what();
        return 1;
    }

    return 0;
}
