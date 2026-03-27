#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/CompositorAppInterface.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace horizon
{

    class DockShelf;
    class DockItem;
    class Menu;

    struct PinnedApp
    {
        std::string app_id;
        std::string name;
        std::string icon;
        std::string run_id;
    };

    class DockApplication : public Application
    {
    public:
        DockApplication();
        ~DockApplication() override;

        WaylandLayerWindow *window() const { return m_window; }

        // Creates the base context menu for a dock item.
        std::unique_ptr<Menu> create_context_menu(DockItem *item);

    private:
        void detect_environment();
        void setup_ui();
        void setup_ipc();
        void update_dock(const std::vector<ApplicationInfo> &apps);

        // Configuration watching
        void load_config();
        void start_watcher();
        void stop_watcher();
        void watch_loop();

        bool _is_wayfire = false;
        WaylandLayerWindow *m_window = nullptr;
        DockShelf *_shelf_ptr = nullptr;
        std::unique_ptr<CompositorAppInterface> _compositor_apps;
        static const std::vector<PinnedApp> PINNED_APPS;

        std::string m_config_path;
        int inotify_fd = -1;
        int watch_fd = -1;
        std::thread watcher_thread;
        std::atomic<bool> running{false};

        // Last known app list — used to rebuild icons after a config change
        std::vector<ApplicationInfo> m_last_apps;

    };

} // namespace horizon
