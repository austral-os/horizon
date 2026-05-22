#include "DockApplication.hpp"
#include "DockItem.hpp"
#include "DockShelf.hpp"
#include "horizon/EventsManager.hpp"
#include <algorithm>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/DesktopManager.hpp>
#include <horizon/LabwcAppAdapter.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WayfireAppAdapter.hpp>
#include <horizon/Widget.hpp>
#include <horizon/I18n.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace horizon
{

    DockApplication::DockApplication() : Application("org.horizon.dock", 800, 160, true, true)
    {
        // Load translations
        i18n().load_app_locales("dock");

        m_pinned_apps = {};
        m_window = create_layer_window("org.horizon.dock", 2); // ZWLR_LAYER_SHELL_V1_LAYER_TOP

        m_window->set_name(i18n().tr("dock.title"));
        m_window->set_anchor_and_size(2 | 4 | 8, 0, 160); // BOTTOM | LEFT | RIGHT
        m_window->set_exclusive_zone(100);
        m_window->set_show_in_dock(false);
        m_window->set_show_in_system_tray(false);

        // Setup About info
        auto &about = about_manager();
        about.set_app_title("Horizon Dock");
        about.set_app_description("Horizon Dock provides a quick way to launch and switch between applications.");
        about.set_app_version(APP_VERSION);
        about.set_app_icon("org.horizon.dock");
        about.set_app_git(ABOUT_HORIZON.git);
        m_window->set_visible(true);
        m_window->set_keyboard_interactivity(0);

        // Enable background blur
        m_window->set_blur(true);

        // Add custom search path for .desktop files if in development mode
        bool is_dev = false;
        try {
            auto exe_path = std::filesystem::read_symlink("/proc/self/exe");
            is_dev = exe_path.string().find("/build/") != std::string::npos;
        } catch (...) {}

        if (is_dev) {
            DesktopManager::add_search_path(std::string(HORIZON_SOURCE_DIR) + "/examples/config/apps/");
        }

        detect_environment();
        setup_ui();

        const char* home = std::getenv("HOME");
        if (home)
        {
            m_config_path = std::string(home) + "/.config/horizon/dock.json";
        }

        load_config();
        start_watching(m_config_path);

        setup_ipc();
    }

    DockApplication::~DockApplication()
    {
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

        root->when_mouse_enter.connect([this](auto &) {
            if (!m_autohide_enabled) return;
            LOG_INFO << "[DOCK] Mouse entered dock area. m_is_hidden=" << m_is_hidden << ", time=" << m_autohide_time;
            if (m_autohide_timer != 0)
            {
                m_window->stop_timer(m_autohide_timer);
                m_autohide_timer = 0;
            }
            if (m_is_hidden && m_autohide_show_timer == 0)
            {
                if (m_autohide_time > 0)
                {
                    LOG_INFO << "[DOCK] Starting show timer for " << m_autohide_time << " ms";
                    m_autohide_show_timer = m_window->add_timer(m_autohide_time, [this]() {
                        LOG_INFO << "[DOCK] Show timer triggered!";
                        m_is_hidden = false;
                        apply_autohide_state();
                        m_autohide_show_timer = 0;
                    });
                }
                else
                {
                    m_is_hidden = false;
                    apply_autohide_state();
                }
            }
        });

        root->when_mouse_leave.connect([this](auto &) {
            if (!m_autohide_enabled) return;
            
            if (m_autohide_show_timer != 0)
            {
                LOG_INFO << "[DOCK] Mouse left dock area before show timer completed. Cancelling show timer.";
                m_window->stop_timer(m_autohide_show_timer);
                m_autohide_show_timer = 0;
            }
            
            if (m_autohide_timer == 0 && !m_is_hidden)
            {
                LOG_INFO << "[DOCK] Mouse left dock area. Starting hide timer (300 ms).";
                m_autohide_timer = m_window->add_timer(300, [this]() {
                    LOG_INFO << "[DOCK] Hide timer triggered!";
                    m_is_hidden = true;
                    apply_autohide_state();
                    m_autohide_timer = 0;
                });
            }
        });

        auto left_spacer = std::make_unique<Widget>();
        auto right_spacer = std::make_unique<Widget>();

        auto shelf = std::make_unique<DockShelf>();
        shelf->set_dock_app(this);
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

        const auto &instances = item->instances();
        std::string run_id = item->run_id();
        std::string app_id = item->app_id();

        if (!instances.empty() || !app_id.empty())
        {
            // 1. Open new instance
            std::string launch_id = run_id.empty() ? app_id : run_id;
            if (!launch_id.empty())
            {
                auto *new_instance_item = menu->add_item(i18n().tr("dock.context.open"));
                new_instance_item->when_click.connect(
                    [launch_id](auto &)
                    {
                        ApplicationLauncher::launch(launch_id);
                    });
            }

            // Pin/Unpin logic (at the top for visibility)
            bool is_pinned = false;
            for (const auto &pinned : m_pinned_apps)
            {
                if (!app_id.empty() && 
                    (app_id == pinned.app_id || 
                     app_id.find(pinned.app_id + ".") == 0 ||
                     pinned.app_id.find(app_id + ".") == 0))
                {
                    is_pinned = true;
                    break;
                }
            }

            menu->add_separator();
            if (is_pinned)
            {
                auto *unpin_item = menu->add_item(i18n().tr("dock.context.unpin"));
                unpin_item->when_click.connect(
                    [this, app_id](auto &)
                    {
                        unpin_app(app_id);
                    });
            }
            else
            {
                auto *pin_item = menu->add_item(i18n().tr("dock.context.pin"));
                std::string pin_id = app_id.empty() ? run_id : app_id;
                std::string pin_run_id = run_id.empty() ? app_id : run_id;
                
                std::string pin_name = item->instances().empty() ? pin_id : item->instances()[0].title;
                if (pin_name.empty()) pin_name = pin_id;
                std::string pin_icon = item->icon_name();
                if (pin_icon.empty()) pin_icon = pin_id;

                // Attempt to resolve better metadata from desktop entry
                std::string desktop_path = DesktopManager::find_desktop_file(pin_id);
                if (!desktop_path.empty()) {
                    std::string d_name = DesktopManager::get_value_from_desktop_file(desktop_path, "Name");
                    if (!d_name.empty()) pin_name = d_name;
                    
                    std::string d_icon = DesktopManager::get_value_from_desktop_file(desktop_path, "Icon");
                    if (!d_icon.empty()) pin_icon = d_icon;

                    // If it's a desktop file, the run_id should be its ID (filename without .desktop)
                    std::filesystem::path p(desktop_path);
                    std::string stem = p.stem().string();
                    if (!stem.empty()) pin_run_id = stem;
                }

                pin_item->when_click.connect(
                    [this, pin_id, pin_name, pin_icon, pin_run_id](auto &)
                    {
                        pin_app(pin_id, pin_name, pin_icon, pin_run_id);
                    });
            }

            if (!instances.empty())
            {
                menu->add_separator();
                
                // 2. List all open windows/instances
                for (const auto &info : instances)
                {
                    std::string title = info.title.empty() ? i18n().tr("dock.context.untitled") : info.title;
                    auto *window_item = menu->add_item(title);
                    
                    struct zwlr_foreign_toplevel_handle_v1 *handle = info.handle;
                    int pid = info.pid;
                    std::string aid = info.app_id;

                    window_item->when_click.connect(
                        [this, handle, pid, aid](auto &)
                        {
                            if (handle != nullptr) {
                                _compositor_apps->activate_instance(handle);
                            } else if (pid != -1) {
                                m_window->send_remote_signal(pid, "activate");
                            }
                        });
                }

                menu->add_separator();

                // 3. Global actions for the app
                auto *exit_item = menu->add_item(i18n().tr("dock.context.quit"));
                exit_item->when_click.connect(
                    [this, app_id](auto &)
                    {
                        if (!app_id.empty() && app_id != "org.horizon.dock" && app_id != m_app_id) {
                            _compositor_apps->close(app_id);
                        } else {
                            LOG_ERROR << "[DOCK] REJECTED close request for protected ID: '" << app_id << "'";
                        }
                    });
            }
        }

        return menu;
    }

    void DockApplication::update_dock(const std::vector<ApplicationInfo> &apps)
    {
        m_last_apps = apps;
        if (_shelf_ptr) {
            _shelf_ptr->cancel_drag();
            _shelf_ptr->clear_children();
        }

        // 1. Group all apps by app_id
        std::map<std::string, std::vector<ApplicationInfo>> grouped_running_apps;
        for (const auto &app_info : apps)
        {
            if (app_info.show_in_dock)
            {
                // PROTECT DOCK AGAINST SELF-SHOWING
                if (app_info.app_id == "org.horizon.dock" || app_info.app_id == m_app_id)
                {
                    continue;
                }

                grouped_running_apps[app_info.app_id].push_back(app_info);
            }
        }

        std::set<std::string> handled_app_ids;

        // 2. Add Pinned Apps
        for (const auto &pinned : m_pinned_apps)
        {
            auto item = std::make_unique<DockItem>(m_window, _compositor_apps.get(), pinned.icon, _is_wayfire);
            item->set_run_id(pinned.run_id);
            item->set_app_id(pinned.app_id);
            item->set_name(pinned.name);

            // Check if any running app matches this pinned app_id
            bool is_running = false;
            for (auto it = grouped_running_apps.begin(); it != grouped_running_apps.end(); ++it)
            {
                // Robust matching: exact match or matches as a dot-separated component (e.g. "firefox" matches "firefox.desktop")
                if (it->first == pinned.app_id || 
                    it->first.find(pinned.app_id + ".") == 0 || 
                    pinned.app_id.find(it->first + ".") == 0)
                {
                    is_running = true;
                    for (const auto& info : it->second)
                    {
                        item->add_instance(info);
                    }
                    handled_app_ids.insert(it->first);
                }
            }

            if (!is_running)
            {
                item->set_pinned_data(pinned.run_id);
            }

            item->set_position_type(FREE);
            item->set_context_menu(create_context_menu(item.get()));
            _shelf_ptr->add_child(std::move(item));
        }

        // 3. Add Separator if there are other running apps
        bool has_other_apps = false;
        for (const auto &pair : grouped_running_apps)
        {
            if (handled_app_ids.find(pair.first) == handled_app_ids.end())
            {
                has_other_apps = true;
                break;
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

        // 4. Add Other Running Apps (unpinned)
        for (auto &pair : grouped_running_apps)
        {
            const std::string& app_id = pair.first;
            if (handled_app_ids.find(app_id) != handled_app_ids.end())
                continue;

            const auto& instances = pair.second;
            if (instances.empty()) continue;

            std::string icon = instances[0].icon;
            if (icon.empty()) icon = app_id;

            auto item = std::make_unique<DockItem>(m_window, _compositor_apps.get(), icon, _is_wayfire);
            for (const auto& info : instances)
            {
                item->add_instance(info);
            }

            // For unpinned apps, we don't automatically assign a run_id from pinned apps
            // unless it's a very clear match, to avoid confusion in the context menu.
            // If the user wants to pin it, we'll use the app_id as run_id.
            item->set_app_id(app_id);

            item->set_position_type(FREE);
            item->set_context_menu(create_context_menu(item.get()));
            _shelf_ptr->add_child(std::move(item));
        }

        _shelf_ptr->calculate_layout();
        if (m_window->root())
            m_window->root()->calculate_layout();
        m_window->invalidate();

    }

    void DockApplication::load_config()
    {
        if (m_config_path.empty()) return;

        std::string system_path = "/usr/share/horizon/dock.json";

        try {
            if (!std::filesystem::exists(m_config_path)) {
                LOG_INFO << "[DOCK] User config not found, checking for system default at: " << system_path;
                if (std::filesystem::exists(system_path)) {
                    std::filesystem::create_directories(std::filesystem::path(m_config_path).parent_path());
                    std::filesystem::copy_file(system_path, m_config_path);
                    LOG_INFO << "[DOCK] Successfully copied system default config to: " << m_config_path;
                } else {
                    LOG_ERROR << "[DOCK] System default config NOT found at: " << system_path;
                    // If no user and no system config, we'll just return and stay with empty pinned apps
                    return;
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "[DOCK] Error during config migration: " << e.what();
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
                m_autohide_enabled = dock_config.value("autohide", false);
                m_autohide_time = dock_config.value("autohide-time", 0);
                m_position = dock_config.value("position", "bottom");

                if (_shelf_ptr)
                {
                    _shelf_ptr->set_base_size(icon_size);
                    _shelf_ptr->set_magnification_enabled(magnification_enabled);
                    _shelf_ptr->set_dock_position(m_position);
                    // Proportional sizing
                    m_total_height = static_cast<int>(icon_size * 2.5f);
                    int exclusive_zone = m_autohide_enabled ? 0 : static_cast<int>(icon_size * 1.5625f);
                    LOG_INFO << "[DOCK] Configuration reloaded. New total_height: " << m_total_height << ", window width: " << m_window->width();
                    
                    if (m_window)
                    {
                        if (m_position == "left") {
                            m_window->set_anchor_and_size(4 | 1 | 2, m_total_height, 0); // LEFT | TOP | BOTTOM
                            m_window->root()->set_layout_type(WIDGET_LAYOUT_VERTICAL);
                        } else if (m_position == "right") {
                            m_window->set_anchor_and_size(8 | 1 | 2, m_total_height, 0); // RIGHT | TOP | BOTTOM
                            m_window->root()->set_layout_type(WIDGET_LAYOUT_VERTICAL);
                        } else {
                            m_window->set_anchor_and_size(2 | 4 | 8, 0, m_total_height); // BOTTOM | LEFT | RIGHT
                            m_window->root()->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                        }
                        m_window->set_exclusive_zone(exclusive_zone);
                        
                        // Default to hidden when autohide is just enabled
                        if (m_autohide_enabled && !m_is_hidden && m_autohide_timer == 0) {
                            m_is_hidden = true;
                        }
                        apply_autohide_state();
                        
                        // Force local synchronization of root widget size to ensure hit-testing
                        // works correctly before the compositor responds with a configure event.
                        if (m_position == "left" || m_position == "right") {
                            m_window->on_resize(m_total_height, m_window->height());
                        } else {
                            m_window->on_resize(m_window->width(), m_total_height);
                        }
                    }

                    // Recreate all dock icons at the new size to get fresh rendering state
                    update_dock(m_last_apps);
                }

                if (dock_config.contains("pinned"))
                {
                    m_pinned_apps.clear();
                    for (const auto &p : dock_config["pinned"])
                    {
                        PinnedApp app;
                        app.app_id = p.value("app_id", "");
                        app.name = p.value("name", "");
                        app.icon = p.value("icon", "");
                        app.run_id = p.value("run_id", "");
                        if (!app.app_id.empty())
                        {
                            m_pinned_apps.push_back(app);
                        }
                    }
                    update_dock(m_last_apps);
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "[DOCK] Error parsing JSON: " << e.what();
        }
    }

    void DockApplication::save_config()
    {
        if (m_config_path.empty()) return;

        try
        {
            nlohmann::json j;
            if (std::filesystem::exists(m_config_path))
            {
                std::ifstream file(m_config_path);
                file >> j;
            }

            auto &dock_config = j["dock"];
            
            nlohmann::json pinned_json = nlohmann::json::array();
            for (const auto &app : m_pinned_apps)
            {
                nlohmann::json p;
                p["app_id"] = app.app_id;
                p["name"] = app.name;
                p["icon"] = app.icon;
                p["run_id"] = app.run_id;
                pinned_json.push_back(p);
            }
            dock_config["pinned"] = pinned_json;

            // Ensure directory exists
            std::filesystem::path p(m_config_path);
            if (!std::filesystem::exists(p.parent_path()))
            {
                std::filesystem::create_directories(p.parent_path());
            }

            std::ofstream out(m_config_path);
            out << j.dump(4);
            LOG_INFO << "[DOCK] Configuration saved to: " << m_config_path;
        }
        catch (const std::exception &e)
        {
            LOG_ERROR << "[DOCK] Error saving JSON: " << e.what();
        }
    }

    void DockApplication::pin_app(const std::string &app_id, const std::string &name, const std::string &icon, const std::string &run_id)
    {
        // Check if already pinned
        for (const auto &pinned : m_pinned_apps)
        {
            if (pinned.app_id == app_id) return;
        }

        PinnedApp app;
        app.app_id = app_id;
        app.name = name.empty() ? app_id : name;
        app.icon = icon.empty() ? app_id : icon;
        app.run_id = run_id.empty() ? app_id : run_id;
        m_pinned_apps.push_back(app);

        save_config();
        update_dock(m_last_apps);
    }

    void DockApplication::pin_app_at(const std::string &app_id, const std::string &name, const std::string &icon, const std::string &run_id, int index)
    {
        // Check if already pinned
        auto it = std::find_if(m_pinned_apps.begin(), m_pinned_apps.end(),
                               [&app_id](const PinnedApp &a) { return a.app_id == app_id; });
        
        if (it != m_pinned_apps.end()) {
            // If already pinned, just move it to the new position if different
            int old_index = std::distance(m_pinned_apps.begin(), it);
            reorder_pinned_app(old_index, index);
            return;
        }

        PinnedApp app;
        app.app_id = app_id;
        app.name = name.empty() ? app_id : name;
        app.icon = icon.empty() ? app_id : icon;
        app.run_id = run_id.empty() ? app_id : run_id;

        if (index < 0 || index >= (int)m_pinned_apps.size()) {
            m_pinned_apps.push_back(app);
        } else {
            m_pinned_apps.insert(m_pinned_apps.begin() + index, app);
        }

        save_config();
        update_dock(m_last_apps);
    }

    void DockApplication::unpin_app(const std::string &app_id)
    {
        auto it = std::remove_if(m_pinned_apps.begin(), m_pinned_apps.end(),
                                 [&app_id](const PinnedApp &a) { 
                                     // Check for exact match or namespaced match
                                     return a.app_id == app_id || 
                                            a.app_id.find(app_id + ".") == 0 ||
                                            app_id.find(a.app_id + ".") == 0;
                                 });
        
        if (it != m_pinned_apps.end())
        {
            m_pinned_apps.erase(it, m_pinned_apps.end());
            save_config();
            update_dock(m_last_apps);
        }
    }

    void DockApplication::reorder_pinned_app(int old_index, int new_index)
    {
        if (old_index == new_index || 
            old_index < 0 || old_index >= (int)m_pinned_apps.size() ||
            new_index < 0 || new_index >= (int)m_pinned_apps.size()) {
            return;
        }

        PinnedApp app = m_pinned_apps[old_index];
        m_pinned_apps.erase(m_pinned_apps.begin() + old_index);
        m_pinned_apps.insert(m_pinned_apps.begin() + new_index, app);

        save_config();
        update_dock(m_last_apps);
    }

    void DockApplication::on_file_changed()
    {
        LOG_INFO << "[DOCK] Config change detected, reloading...";
        load_config();
    }

    void DockApplication::post_watcher_task(std::function<void()> task)
    {
        if (m_window)
        {
            m_window->post_task(task);
        }
    }


    void DockApplication::apply_autohide_state()
    {
        if (!m_window) return;

        if (m_autohide_enabled && m_is_hidden)
        {
            if (m_position == "left") {
                m_window->set_margin(0, 0, 0, -(m_total_height - 1));
            } else if (m_position == "right") {
                m_window->set_margin(0, -(m_total_height - 1), 0, 0);
            } else {
                m_window->set_margin(0, 0, -(m_total_height - 1), 0);
            }
        }
        else
        {
            m_window->set_margin(0, 0, 0, 0);
        }
    }

} // namespace horizon
