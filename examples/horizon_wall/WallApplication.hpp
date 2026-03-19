#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <string>

namespace horizon
{
    class WallApplication : public Application
    {
    public:
        WallApplication(const std::string &wall_path = "");
        ~WallApplication() override;

    private:
        void setup_window();
        void load_wallpaper(const std::string &wall_path);

        void start_watcher();
        void stop_watcher();
        void watch_loop();

        WaylandLayerWindow *m_window{nullptr};
        std::string m_wall_path;
        std::string m_config_path;

        int inotify_fd{-1};
        int watch_fd{-1};
        std::atomic<bool> running{false};
        std::thread watcher_thread;
    };
} // namespace horizon
