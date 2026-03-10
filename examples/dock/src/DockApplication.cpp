#include "DockApplication.hpp"
#include "DockItem.hpp"
#include "DockShelf.hpp"
#include <algorithm>
#include <horizon/DesktopEntry.hpp>
#include <horizon/LabwcAppAdapter.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WayfireAppAdapter.hpp>
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
        : LayerApplication("org.horizon.dock", ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY)
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
            if (is_running)
            {
                // We'll need to update DockItem to accept ApplicationInfo or keep JSON
                // For now, let's assume we update DockItem
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
                item->set_app_info(app_info);
                _shelf_ptr->add_child(std::move(item));
            }
        }

        _shelf_ptr->calculate_layout();
        invalidate();
    }

} // namespace horizon
