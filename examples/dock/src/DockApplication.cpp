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
#include <horizon/Window.hpp>
#include <horizon/LayerWindow.hpp>
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
        main_window()->set_background_color({0.0f, 0.0f, 0.0f, 0.0f});

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

        main_window()->add_child(std::move(root));
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
                        else if (item_id.find("dock_exit_id:") == 0)
                        {
                            std::string app_id = item_id.substr(13);
                            LOG_INFO << "[DOCK] Exit requested for app_id: " << app_id;
                            compositor_apps()->close(app_id);
                        }
                        else if (item_id.find("dock_fullscreen:") == 0)
                        {
                            int pid = std::stoi(item_id.substr(16));
                            LOG_INFO << "[DOCK] Fullscreen toggle requested for pid: " << pid;
                            send_remote_signal(pid, "toggle_fullscreen");
                        }
                        else if (item_id.find("dock_fullscreen_id:") == 0)
                        {
                            std::string app_id = item_id.substr(19);
                            LOG_INFO << "[DOCK] Fullscreen toggle requested for app_id: " << app_id;
                            compositor_apps()->toggle_fullscreen(app_id);
                        }
                        else if (item_id.find("dock_launch:") == 0)
                        {
                            std::string run_id = item_id.substr(12);
                            LOG_INFO << "[DOCK] Launch requested for run_id: " << run_id;
                            ApplicationLauncher::launch(run_id);
                        }
                        else if (item_id.find("dock_exit_instance:") == 0)
                        {
                            uintptr_t instance_id = std::stoull(item_id.substr(19));
                            LOG_INFO << "[DOCK] Exit requested for instance: " << instance_id;
                            compositor_apps()->close_instance(instance_id);
                        }
                        else if (item_id.find("dock_fullscreen_instance:") == 0)
                        {
                            uintptr_t instance_id = std::stoull(item_id.substr(25));
                            LOG_INFO << "[DOCK] Fullscreen toggle requested for instance: " << instance_id;
                            compositor_apps()->toggle_fullscreen_instance(instance_id);
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

    void DockApplication::show_dock_context_menu(int x, int y, int pid, const std::string &run_id, const std::string &app_id, uintptr_t instance_id)
    {
        // La coordenada 'y' recibida es local dentro del dock.
        // El dock está anclado al borde inferior, así que su posición global en Y
        // es monitor_height - dock_height. Sumamos ese offset para la coordenada
        // global de pantalla que espera el menu manager.
        // La coordenada 'x' recibida es local al shelf. Debemos sumar shelf->x() 
        // para tener la coordenada local a la ventana del Dock, que al estar 
        // anclada a (0, screen_h - dock_h) y ocupar todo el ancho, coincide
        // con la coordenada global X de la pantalla.
        int screen_h = w_surface()->monitor_height();
        int dock_height = height();
        int global_x = x;
        int global_y = y;

        if (screen_h > 0)
        {
            global_y = (screen_h - dock_height) + y;
            LOG_INFO << "[DOCK] screen_h=" << screen_h << " dock_h=" << dock_height
                     << " global_x=" << global_x << " local_y=" << y << " global_y=" << global_y;
        }
        else
        {
            LOG_INFO << "[DOCK] monitor_height unknown, using local y=" << y 
                     << " global_x=" << global_x;
        }

        // Build the context menu
        auto menu = std::make_unique<Menu>();
        menu->set_title("dock_context");

        if (pid != -1 || !app_id.empty())
        {
            // First option: Open new instance (if we have a way to launch it)
            std::string launch_id = run_id;
            if (launch_id.empty() && !app_id.empty())
            {
                launch_id = app_id;
            }

            if (!launch_id.empty())
            {
                auto *new_instance_item = menu->add_item("Abrir nueva instancia");
                new_instance_item->set_id("dock_launch:" + launch_id);
            }

            // Second option: Fullscreen
            auto *fullscreen_item = menu->add_item("Entrar en pantalla completa");
            if (instance_id != 0)
                fullscreen_item->set_id("dock_fullscreen_instance:" + std::to_string(instance_id));
            else if (pid != -1)
                fullscreen_item->set_id("dock_fullscreen:" + std::to_string(pid));
            else
                fullscreen_item->set_id("dock_fullscreen_id:" + app_id);

            // Separator
            menu->add_separator();

            // Third option: Exit
            auto *exit_item = menu->add_item("Salir");
            if (instance_id != 0)
                exit_item->set_id("dock_exit_instance:" + std::to_string(instance_id));
            else if (pid != -1)
                exit_item->set_id("dock_exit:" + std::to_string(pid));
            else
                exit_item->set_id("dock_exit_id:" + app_id);
        }
        else if (!run_id.empty())
        {
            auto *launch_item = menu->add_item("Lanzar aplicación");
            launch_item->set_id("dock_launch:" + run_id);
        }

        LOG_INFO << "[DOCK] Showing context menu at (" << global_x << ", " << global_y << ") for pid=" << pid;
        _client_menu.show_menu(menu.get(), global_x, global_y, -1, "org.horizon.dock");

        // menu is kept alive long enough for show_menu() to serialize it
    }

    void DockApplication::update_dock(const std::vector<ApplicationInfo> &apps)
    {
        LOG_INFO << "[DOCK-DEBUG] update_dock called with " << apps.size() << " apps.";
        for (const auto &app : apps)
        {
            LOG_INFO << "[DOCK-DEBUG] Received App: id='" << app.app_id << "' title='" << app.title
                     << "' icon='" << app.icon << "' pid=" << app.pid
                     << " show_in_dock=" << (app.show_in_dock ? "yes" : "no");
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
            { show_dock_context_menu(x, y, item_ptr->pid(), item_ptr->run_id(), item_ptr->app_id(), item_ptr->instance_id()); };

            if (is_running)
            {
                item->set_app_info(running_app_data);
                // Also set run_id so we can launch new instances
                item->set_run_id(pinned.run_id);
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
                    // This is an instance of a pinned app that was already handled in Step 1
                    continue;
                }

                std::string icon = app_info.icon;
                if (icon.empty())
                    icon = app_info.app_id;

                auto item = std::make_unique<DockItem>(this, icon, _is_wayfire);
                item->set_app_info(app_info);

                // Try to find the run_id for this app_id from PINNED_APPS
                for (const auto &pinned : PINNED_APPS)
                {
                    if (app_info.app_id.find(pinned.app_id) != std::string::npos)
                    {
                        item->set_run_id(pinned.run_id);
                        break;
                    }
                }

                item->on_right_click = [this, item_ptr = item.get()](int x, int y)
                {
                    show_dock_context_menu(x, y, item_ptr->pid(), item_ptr->run_id(),
                                           item_ptr->app_id(), item_ptr->instance_id());
                };
                _shelf_ptr->add_child(std::move(item));
            }
        }

        _shelf_ptr->calculate_layout();
        invalidate();
    }

} // namespace horizon
