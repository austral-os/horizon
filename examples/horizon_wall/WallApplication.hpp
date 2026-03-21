#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <string>

namespace horizon
{
    class Image;
    class WallApplication : public Application
    {
    public:
        WallApplication(const std::string &wall_path = "");
        ~WallApplication() override;

    private:
        void setup_window();
        void load_wallpaper(const std::string &wall_path);

        void start_gallery(const std::string &directory, int interval_ms);
        void stop_gallery();
        void next_gallery_image();

        void start_watcher();
        void stop_watcher();
        void watch_loop();

        WaylandLayerWindow *m_window{nullptr};
        Image *m_wallpaper_widget{nullptr};

        size_t m_gallery_timer_id{0};
        std::vector<std::string> m_gallery_images;
        size_t m_current_gallery_index{0};

        std::string m_wall_path;
        std::string m_config_path;

        int inotify_fd{-1};
        int watch_fd{-1};
        std::atomic<bool> running{false};
        std::thread watcher_thread;
    };
} // namespace horizon
