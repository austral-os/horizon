#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/CompositorAppInterface.hpp>
#include <horizon/FileWatcher.hpp>
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

    class DockApplication : public Application, public FileWatcher
    {
    public:
        DockApplication();
        ~DockApplication() override;

    public:
        WaylandLayerWindow *window() const { return m_window; }

        // Creates the base context menu for a dock item.
        std::unique_ptr<Menu> create_context_menu(DockItem *item);

        void pin_app(const std::string &app_id, const std::string &name, const std::string &icon, const std::string &run_id);
        void pin_app_at(const std::string &app_id, const std::string &name, const std::string &icon, const std::string &run_id, int index);
        void unpin_app(const std::string &app_id);
        void reorder_pinned_app(int old_index, int new_index);

    private:
        void detect_environment();
        void setup_ui();
        void setup_ipc();
        void update_dock(const std::vector<ApplicationInfo> &apps);
        void save_config();
        void load_config();

        // FileWatcher overrides
        void on_file_changed() override;
        void post_watcher_task(std::function<void()> task) override;

        void apply_autohide_state();

        bool _is_wayfire = false;
        WaylandLayerWindow *m_window = nullptr;
        DockShelf *_shelf_ptr = nullptr;
        std::unique_ptr<CompositorAppInterface> _compositor_apps;
        std::vector<PinnedApp> m_pinned_apps;

        std::string m_config_path;

        // Last known app list — used to rebuild icons after a config change
        std::vector<ApplicationInfo> m_last_apps;
        
        bool m_autohide_enabled = false;
        bool m_is_hidden = false;
        size_t m_autohide_timer = 0;
        size_t m_autohide_show_timer = 0;
        int m_autohide_time = 0;
        int m_total_height = 160;

    };

} // namespace horizon
