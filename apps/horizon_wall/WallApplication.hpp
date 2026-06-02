#pragma once

#include <horizon/Application.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Menu.hpp>
#include <string>
#include <vector>
#include <memory>

namespace horizon
{
    class Image;
    namespace files {
        class FileIconView;
    }
    class WallApplication : public Application
    {
    public:
        WallApplication(const std::string &wall_path = "");
        ~WallApplication() override;

    private:
        void setup_window(WaylandLayerWindow* window);
        void setup_global_menu(WaylandLayerWindow* window);
        void load_wallpaper(const std::string &wall_path);

        void start_gallery(const std::string &directory, int interval_ms, const std::string &order = "none");
        void stop_gallery();
        void next_gallery_image();

        void start_watcher();
        void stop_watcher();
        void watch_loop();

        std::vector<WaylandLayerWindow *> m_windows;
        std::vector<Image *> m_wallpaper_widgets;
        std::vector<horizon::files::FileIconView*> m_icon_views;
        std::vector<std::unique_ptr<Menu>> m_global_menus_storage;

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
