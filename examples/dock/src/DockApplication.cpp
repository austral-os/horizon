#include "DockApplication.hpp"
#include "DockItem.hpp"
#include "DockShelf.hpp"
#include "horizon/EventsManager.hpp"
#include <algorithm>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/DesktopEntry.hpp>
#include <horizon/LabwcAppAdapter.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WayfireAppAdapter.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace horizon
{

    const std::vector<PinnedApp> DockApplication::PINNED_APPS = {
        {"org.horizon.launchpad", "Launchpad", "slingscold", "launchpad"},
        {"arkfm", "Ark File Manager", "arkfm", "arkfm"},
        {"alacritty", "Terminal", "utilities-terminal", "terminal"},
        {"firefox", "Web Browser", "firefox", "firefox"},
        {"horizon.preferences", "Preferences", "applications-system", "preferences"}};

    DockApplication::DockApplication() : Application("org.horizon.dock", 800, 160, true, true)
    {
        m_window = create_layer_window("org.horizon.dock", 2); // ZWLR_LAYER_SHELL_V1_LAYER_TOP

        m_window->set_name("Dock");
        m_window->set_anchor(2 | 4 | 8); // BOTTOM | LEFT | RIGHT
        m_window->set_size(0, 160);
        m_window->set_exclusive_zone(100);
        m_window->set_show_in_dock(false);
        m_window->set_show_in_system_tray(false);
        m_window->set_visible(true);
        m_window->set_keyboard_interactivity(0);

        // Add custom search path for .desktop files
        DesktopEntry::add_search_path(
            "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/");

        detect_environment();
        setup_ui();

        const char* home = std::getenv("HOME");
        if (home)
        {
            m_config_path = std::string(home) + "/.config/horizon/horizon.json";
        }

        load_config();
        start_watcher();

        setup_ipc();
    }

    DockApplication::~DockApplication()
    {
        stop_watcher();
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
            _compositor_apps = std::make_unique<WayfireAppAdapter>(m_window);
        }
        else
        {
            LOG_INFO << "[DOCK] Labwc detected, using LabwcAppAdapter.";
            _compositor_apps = std::make_unique<LabwcAppAdapter>(m_window);
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
            { m_window->post_task([this, apps = ctx.apps]() { update_dock(apps); }); });
    }

    std::unique_ptr<Menu> DockApplication::create_context_menu(DockItem *item)
    {
        auto menu = std::make_unique<Menu>();
        menu->set_title("dock_context");

        int pid = item->pid();
        std::string run_id = item->run_id();
        std::string app_id = item->app_id();
        uintptr_t instance_id = item->instance_id();

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
                new_instance_item->when_click.connect(
                    [launch_id](auto &)
                    {
                        LOG_INFO << "[DOCK] Launch requested code for run_id: " << launch_id;
                        ApplicationLauncher::launch(launch_id);
                    });
            }

            // Second option: Fullscreen
            auto *fullscreen_item = menu->add_item("Entrar en pantalla completa");
            fullscreen_item->when_click.connect(
                [this, instance_id, pid, app_id](auto &)
                {
                    if (instance_id != 0)
                        _compositor_apps->toggle_fullscreen_instance(instance_id);
                    else if (pid != -1)
                        m_window->send_remote_signal(pid, "toggle_fullscreen");
                    else
                        _compositor_apps->toggle_fullscreen(app_id);
                });

            // Separator
            menu->add_separator();

            // Third option: Exit
            auto *exit_item = menu->add_item("Salir");
            exit_item->when_click.connect(
                [this, instance_id, pid, app_id](auto &)
                {
                    if (instance_id != 0)
                        _compositor_apps->close_instance(instance_id);
                    else if (pid != -1)
                        m_window->send_remote_signal(pid, "close");
                    else
                        _compositor_apps->close(app_id);
                });
        }
        else if (!run_id.empty())
        {
            auto *launch_item = menu->add_item("Lanzar aplicación");
            launch_item->when_click.connect(
                [run_id](auto &)
                {
                    LOG_INFO << "[DOCK] Launch requested for run_id: " << run_id;
                    ApplicationLauncher::launch(run_id);
                });
        }

        return menu;
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

            auto item = std::make_unique<DockItem>(m_window, pinned.icon, _is_wayfire);
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
            item->set_position_type(FREE);
            item->set_context_menu(create_context_menu(item.get()));
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
            separator->set_position_type(FREE);
            _shelf_ptr->add_child(std::move(separator));
        }

        // 3. Add Other Running Apps
        for (const auto &app_info : apps)
        {
            if (app_info.show_in_dock)
            {
                if (running_pinned_ids.find(app_info.app_id) != running_pinned_ids.end())
                {
                    // This is an additional instance of a pinned app
                    // We still show it if it's not the primary one pinned at the start
                    // But we want to keep it grouped? Uniquely identified?
                    // For now, continue showing it as a separate icon as requested by behavior
                }

                std::string icon = app_info.icon;
                if (icon.empty())
                    icon = app_info.app_id;

                auto item = std::make_unique<DockItem>(m_window, icon, _is_wayfire);
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

                item->set_position_type(FREE);
                item->set_context_menu(create_context_menu(item.get()));
                _shelf_ptr->add_child(std::move(item));
            }
        }

        _shelf_ptr->calculate_layout();
        m_window->invalidate();

    }

    void DockApplication::load_config()
    {
        if (m_config_path.empty() || !std::filesystem::exists(m_config_path))
        {
            LOG_ERROR << "[DOCK] Config file not found: " << m_config_path;
            return;
        }

        try
        {
            std::ifstream file(m_config_path);
            nlohmann::json j;
            file >> j;

            if (j.contains("dock"))
            {
                const auto &dock_config = j["dock"];
                int icon_size = dock_config.value("icon_size", 64);
                bool magnification_enabled = dock_config.value("magnification_enabled", true);

                if (_shelf_ptr)
                {
                    _shelf_ptr->set_base_size(icon_size);
                    _shelf_ptr->set_magnification_enabled(magnification_enabled);
                    LOG_INFO << "[DOCK] Loaded config: icon_size=" << icon_size 
                             << ", magnification=" << (magnification_enabled ? "on" : "off");
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "[DOCK] Error parsing JSON: " << e.what();
        }
    }

    void DockApplication::start_watcher()
    {
        if (m_config_path.empty()) return;

        inotify_fd = inotify_init();
        if (inotify_fd < 0)
        {
            LOG_ERROR << "[DOCK] Failed to initialize inotify";
            return;
        }

        watch_fd = inotify_add_watch(inotify_fd, m_config_path.c_str(),
                                     IN_CLOSE_WRITE | IN_MOVED_TO);
        
        if (watch_fd < 0)
        {
            LOG_ERROR << "[DOCK] Failed to add watch for: " << m_config_path;
            return;
        }

        running = true;
        watcher_thread = std::thread(&DockApplication::watch_loop, this);
        LOG_INFO << "[DOCK] Started config watcher for: " << m_config_path;
    }

    void DockApplication::stop_watcher()
    {
        running = false;
        if (watcher_thread.joinable())
        {
            watcher_thread.join();
        }

        if (watch_fd >= 0)
        {
            inotify_rm_watch(inotify_fd, watch_fd);
        }

        if (inotify_fd >= 0)
        {
            close(inotify_fd);
        }
    }

    void DockApplication::watch_loop()
    {
        char buffer[1024];
        struct pollfd pfd = {inotify_fd, POLLIN, 0};

        while (running)
        {
            int ret = poll(&pfd, 1, 500); // 500 ms timeout

            if (ret > 0 && (pfd.revents & POLLIN))
            {
                int length = read(inotify_fd, buffer, sizeof(buffer));

                if (length > 0)
                {
                    // Debounce a bit
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    
                    LOG_INFO << "[DOCK] Config change detected, reloading...";
                    m_window->post_task([this]() {
                        load_config();
                    });
                }
            }
        }
    }

} // namespace horizon
