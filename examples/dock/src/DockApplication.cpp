#include "DockApplication.hpp"
#include "DockItem.hpp"
#include "DockShelf.hpp"
#include <algorithm>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/DesktopEntry.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/LabwcAppAdapter.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WayfireAppAdapter.hpp>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <set>

namespace horizon
{

    const std::vector<PinnedApp> DockApplication::PINNED_APPS = {
        {"org.horizon.launchpad", "Launchpad", "slingscold", "launchpad"},
        {"arkfm", "Ark File Manager", "arkfm", "arkfm"},
        {"alacritty", "Terminal", "utilities-terminal", "terminal"},
        {"firefox", "Web Browser", "firefox", "firefox"}};

    DockApplication::DockApplication()
        : LayerApplication("org.horizon.dock", ZWLR_LAYER_SHELL_V1_LAYER_TOP),
          _router(std::make_unique<RequestRouter>(_message_manager))
    {
        set_name("Dock");
        set_anchor(2 | 4 | 8); // BOTTOM | LEFT | RIGHT
        set_size(0, 100);
        set_exclusive_zone(100);
        set_show_in_dock(false);
        set_show_in_system_tray(false);
        set_visible(true);
        set_keyboard_interactivity(0);

        // Add custom search path for .desktop files
        DesktopEntry::add_search_path(
            "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/");

        detect_environment();
        setup_ui();
        setup_ipc();
    }

    DockApplication::~DockApplication() = default;

    CompositorAppInterface *DockApplication::compositor_apps()
    {
        return _compositor_apps.get();
    }

    void DockApplication::detect_environment()
    {
        const char *desktop_env = getenv("XDG_CURRENT_DESKTOP");
        std::string desktop_str = desktop_env ? desktop_env : "";
        std::transform(desktop_str.begin(), desktop_str.end(), desktop_str.begin(), ::tolower);
        _is_wayfire = (desktop_str.find("wayfire") != std::string::npos);

        if (_is_wayfire)
        {
            LOG_INFO << "[DOCK] Wayfire detected, using WayfireAppAdapter.";
            _compositor_apps = std::make_unique<WayfireAppAdapter>(this);
        }
        else
        {
            LOG_INFO << "[DOCK] Labwc detected, using LabwcAppAdapter.";
            _compositor_apps = std::make_unique<LabwcAppAdapter>(this);
        }
    }

    void DockApplication::setup_ui()
    {
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        root->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});

        auto left_spacer = std::make_unique<Widget>();
        auto right_spacer = std::make_unique<Widget>();

        auto shelf = std::make_unique<DockShelf>();
        _shelf_ptr = shelf.get();

        root->add_child(std::move(left_spacer));
        root->add_child(std::move(shelf));
        root->add_child(std::move(right_spacer));

        set_root(std::move(root));
    }

    void DockApplication::setup_ipc()
    {
        // Initial update
        update_dock(_compositor_apps->get_running_applications());

        _compositor_apps->when_update.connect(
            [this](AppListEventContext &ctx)
            { post_task([this, apps = ctx.apps]() { update_dock(apps); }); });

        setup_context_menu_ipc();
    }

    void DockApplication::setup_context_menu_ipc()
    {
        // Register handler for menu_item_clicked sent back by horizon_menu_manager_d
        _router->register_handler(
            "menu_item_clicked",
            [this](const std::string &request_id, const nlohmann::json &request,
                   MessageManager &) -> nlohmann::json
            {
                std::string item_id = request.value("id", "");
                LOG_INFO << "[DOCK] Context menu item clicked: " << item_id;

                post_task(
                    [this, item_id]()
                    {
                        if (item_id.find("dock_exit:") == 0)
                        {
                            int pid = std::stoi(item_id.substr(10));
                            LOG_INFO << "[DOCK] Exit requested for pid: " << pid;
                            send_remote_signal(pid, "close");
                        }
                        else if (item_id.find("dock_fullscreen:") == 0)
                        {
                            int pid = std::stoi(item_id.substr(16));
                            LOG_INFO << "[DOCK] Fullscreen toggle requested for pid: " << pid;
                            send_remote_signal(pid, "toggle_fullscreen");
                        }
                        else if (item_id.find("dock_launch:") == 0)
                        {
                            std::string run_id = item_id.substr(12);
                            LOG_INFO << "[DOCK] Launch requested for run_id: " << run_id;
                            ApplicationLauncher::launch(run_id);
                        }
                    });

                nlohmann::json response;
                response["status"] = "ok";
                response["request_id"] = request_id;
                return response;
            });

        // Subscribe to the session socket to receive menu_item_clicked messages
        _menu_ipc_client = std::make_unique<IpcClient>("/tmp/horizon_session.sock");
        _menu_ipc_client->subscribe("{\"type\": \"subscribe\"}",
                                    [this](const std::string &msg)
                                    {
                                        try
                                        {
                                            auto j = nlohmann::json::parse(msg);
                                            if (j.value("receiver_id", "") == "org.horizon.dock")
                                            {
                                                std::lock_guard<std::mutex> lock(_queue_mutex);
                                                _pending_messages.push_back(msg);
                                            }
                                        }
                                        catch (...)
                                        {
                                        }
                                    });

        // Process incoming messages on the main thread every 50ms
        add_timer(
            50,
            [this]()
            {
                std::vector<std::string> to_process;
                {
                    std::lock_guard<std::mutex> lock(_queue_mutex);
                    if (_pending_messages.empty())
                        return;
                    to_process = std::move(_pending_messages);
                    _pending_messages.clear();
                }
                for (const auto &msg : to_process)
                {
                    LOG_INFO << "[DOCK] Routing IPC message: " << msg;
                    _router->route(msg);
                }
            },
            true);
    }

    void DockApplication::show_dock_context_menu(int x, int y, int pid, const std::string &run_id)
    {
        // La coordenada 'y' recibida es local dentro del dock.
        // El dock está anclado al borde inferior, así que su posición global en Y
        // es monitor_height - dock_height. Sumamos ese offset para la coordenada
        // global de pantalla que espera el menu manager.
        int screen_h = w_surface()->monitor_height();
        int dock_height = height();
        int global_y = y;
        if (screen_h > 0)
        {
            global_y = (screen_h - dock_height) + y;
            LOG_INFO << "[DOCK] screen_h=" << screen_h << " dock_h=" << dock_height
                     << " local_y=" << y << " global_y=" << global_y;
        }
        else
        {
            LOG_INFO << "[DOCK] monitor_height unknown, using local y=" << y;
        }

        // Build the context menu
        auto menu = std::make_unique<Menu>();
        menu->set_title("dock_context");

        if (pid != -1)
        {
            auto *exit_item = menu->add_item("Salir");
            exit_item->set_id("dock_exit:" + std::to_string(pid));

            auto *fullscreen_item = menu->add_item("Entrar en pantalla completa");
            fullscreen_item->set_id("dock_fullscreen:" + std::to_string(pid));
        }
        else if (!run_id.empty())
        {
            auto *launch_item = menu->add_item("Lanzar aplicación");
            launch_item->set_id("dock_launch:" + run_id);
        }

        LOG_INFO << "[DOCK] Showing context menu at (" << x << ", " << global_y << ") for pid=" << pid;
        _client_menu.show_menu(menu.get(), x, global_y, -1, "org.horizon.dock");

        // menu is kept alive long enough for show_menu() to serialize it
    }

    void DockApplication::update_dock(const std::vector<ApplicationInfo> &apps)
    {
        LOG_INFO << "Updating Dock icons... Found " << apps.size() << " apps.";
        for (const auto &app : apps)
        {
            LOG_INFO << "[DOCK-DEBUG] App: " << app.app_id << " | Title: " << app.title
                     << " | Icon: " << app.icon << " | PID: " << app.pid
                     << " | Active: " << (app.is_active ? "yes" : "no")
                     << " | Minimized: " << (app.is_minimized ? "yes" : "no")
                     << " | ShowInDock: " << (app.show_in_dock ? "yes" : "no");
        }

        _shelf_ptr->clear_children();

        std::set<std::string> running_pinned_ids;

        // 1. Add Pinned Apps
        for (const auto &pinned : PINNED_APPS)
        {
            bool is_running = false;
            ApplicationInfo running_app_data;

            for (const auto &app_info : apps)
            {
                if (app_info.app_id.find(pinned.app_id) != std::string::npos)
                {
                    is_running = true;
                    running_app_data = app_info;
                    running_pinned_ids.insert(app_info.app_id);
                    break;
                }
            }

            auto item = std::make_unique<DockItem>(this, pinned.icon, _is_wayfire);
            item->on_right_click = [this, item_ptr = item.get()](int x, int y)
            { show_dock_context_menu(x, y, item_ptr->pid(), item_ptr->run_id()); };

            if (is_running)
            {
                item->set_app_info(running_app_data);
            }
            else
            {
                item->set_pinned_data(pinned.run_id);
            }
            _shelf_ptr->add_child(std::move(item));
        }

        // 2. Add Separator
        bool has_other_apps = false;
        for (const auto &app_info : apps)
        {
            if (app_info.show_in_dock)
            {
                if (running_pinned_ids.find(app_info.app_id) == running_pinned_ids.end())
                {
                    has_other_apps = true;
                    break;
                }
            }
        }

        if (has_other_apps)
        {
            auto separator = std::make_unique<Widget>();
            separator->set_fixed_size(2);
            separator->set_margin(10);
            separator->set_background_color({1.0f, 1.0f, 1.0f, 0.3f});
            _shelf_ptr->add_child(std::move(separator));
        }

        // 3. Add Other Running Apps
        for (const auto &app_info : apps)
        {
            if (app_info.show_in_dock)
            {
                if (running_pinned_ids.find(app_info.app_id) != running_pinned_ids.end())
                {
                    continue;
                }

                std::string icon = app_info.icon;
                if (icon.empty())
                    icon = app_info.app_id;

                auto item = std::make_unique<DockItem>(this, icon, _is_wayfire);
                item->on_right_click = [this, item_ptr = item.get()](int x, int y)
                { show_dock_context_menu(x, y, item_ptr->pid(), item_ptr->run_id()); };
                item->set_app_info(app_info);
                _shelf_ptr->add_child(std::move(item));
            }
        }

        _shelf_ptr->calculate_layout();
        invalidate();
    }

} // namespace horizon
