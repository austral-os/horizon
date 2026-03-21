#include "WallApplication.hpp"
#include "horizon/EventsManager.hpp"
#include <algorithm>
#include <filesystem>
#include <horizon/Image.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Widget.hpp>
#include <horizon/wlr-layer-shell-unstable-v1-client-protocol.h>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace horizon
{
    WallApplication::WallApplication(const std::string &wall_path)
        : Application("org.horizon.wall", 1920, 1080, true, true),
          m_wall_path(wall_path)
    {
        m_window = create_layer_window("horizon_wall", 0); // Background layer

        m_window->set_name("Horizon Wallpaper");
        m_window->set_icon_name("preferences-desktop-wallpaper");
        m_window->set_show_in_dock(false);

        const char* home = std::getenv("HOME");
        if (home)
        {
            m_config_path = std::string(home) + "/.config/horizon/horizon.json";
        }

        setup_window();
        load_wallpaper(m_wall_path);
        start_watcher();

        m_window->set_visible(true);
        LOG_INFO << "Horizon Wallpaper initialized.";
    }

    WallApplication::~WallApplication()
    {
        stop_gallery();
        stop_watcher();
    }

    void WallApplication::setup_window()
    {
        m_window->set_anchor(
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        m_window->set_exclusive_zone(-1);
    }

    void WallApplication::load_wallpaper(const std::string &wall_path)
    {
        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto wallpaper = std::make_unique<Image>();
        ImageMode mode = ImageMode::Stretch;

        std::string final_path = wall_path;
        std::string type = "image";
        int change_time = 0;

        if (final_path.empty())
        {
            // Try to load from horizon.json
            if (!m_config_path.empty() && std::filesystem::exists(m_config_path))
            {
                try
                {
                    std::ifstream file(m_config_path);
                    nlohmann::json j;
                    file >> j;
                    
                    if (j.contains("desktop") && j["desktop"].contains("backgrounds") && 
                        j["desktop"]["backgrounds"].contains("current"))
                    {
                        const auto& current = j["desktop"]["backgrounds"]["current"];
                        final_path = current.value("path", "");
                        std::string fit = current.value("fit", "fill");
                        type = current.value("type", "image");
                        change_time = current.value("change-time", 0);
                        
                        if (fit == "fill") mode = ImageMode::Stretch;
                        else if (fit == "fit") mode = ImageMode::Fit;
                        else if (fit == "stretch") mode = ImageMode::Stretch;
                        else if (fit == "center") mode = ImageMode::Normal;
                        
                        LOG_INFO << "[HORIZON WALL] Loaded from config: " << final_path << " (fit: " << fit << ", type: " << type << ")";
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "[HORIZON WALL] Error parsing JSON: " << e.what();
                }
                catch (...)
                {
                    LOG_ERROR << "[HORIZON WALL] Unknown error parsing JSON";
                }
            }
        }

        if (final_path.empty())
        {
            std::vector<std::string> search_paths = {
                "examples/config/bg.png",
                "/home/horacio/Desarrollo/austral-os/horizon/examples/config/bg.png"};

            for (const auto &p : search_paths)
            {
                if (std::filesystem::exists(p))
                {
                    final_path = p;
                    break;
                }
            }
        }

        m_wallpaper_widget = wallpaper.get();
        wallpaper->set_mode(mode);

        if (type == "gallery" && !final_path.empty())
        {
            if (std::filesystem::is_directory(final_path))
            {
                start_gallery(final_path, change_time * 1000);
            }
            else if (std::filesystem::exists(final_path))
            {
                // If it's a file, use the parent directory
                start_gallery(std::filesystem::path(final_path).parent_path().string(), change_time * 1000);
            }
        }
        else
        {
            stop_gallery();
            if (!final_path.empty())
            {
                LOG_INFO << "[HORIZON WALL] Loading wallpaper: " << final_path;
                wallpaper->set_path(final_path);
            }
            else
            {
                LOG_ERROR << "[HORIZON WALL] Warning: No wallpaper image found.";
            }
        }

        wallpaper->when_mouse_press.connect(
            [](MouseButtonEventContext &ev)
            { LOG_INFO << "[HORIZON WALL] Wallpaper clicked with button: " << ev.button; });

        root->add_child(std::move(wallpaper));
        m_window->set_root(std::move(root));
    }

    void WallApplication::start_gallery(const std::string &directory, int interval_ms)
    {
        stop_gallery();
        m_gallery_images.clear();
        m_current_gallery_index = 0;

        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            LOG_ERROR << "[HORIZON WALL] Gallery directory does not exist: " << directory;
            return;
        }

        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                {
                    m_gallery_images.push_back(entry.path().string());
                }
            }
        }

        if (m_gallery_images.empty())
        {
            LOG_ERROR << "[HORIZON WALL] No images found in gallery directory: " << directory;
            return;
        }

        std::sort(m_gallery_images.begin(), m_gallery_images.end());

        LOG_INFO << "[HORIZON WALL] Starting gallery with " << m_gallery_images.size() << " images, interval: " << interval_ms << "ms";
        
        // Show the first image immediately
        if (m_wallpaper_widget)
        {
            m_wallpaper_widget->set_path(m_gallery_images[0]);
        }

        if (interval_ms > 0)
        {
            m_gallery_timer_id = m_window->add_timer(interval_ms, [this]() {
                next_gallery_image();
            }, true);
        }
    }

    void WallApplication::stop_gallery()
    {
        if (m_gallery_timer_id != 0)
        {
            m_window->stop_timer(m_gallery_timer_id);
            m_gallery_timer_id = 0;
        }
    }

    void WallApplication::next_gallery_image()
    {
        if (m_gallery_images.empty()) return;

        m_current_gallery_index = (m_current_gallery_index + 1) % m_gallery_images.size();
        
        if (m_wallpaper_widget)
        {
            LOG_INFO << "[HORIZON WALL] Switching to next gallery image: " << m_gallery_images[m_current_gallery_index];
            m_wallpaper_widget->set_path(m_gallery_images[m_current_gallery_index]);
            m_window->invalidate();
        }
    }

    void WallApplication::start_watcher()
    {
        if (m_config_path.empty()) return;

        inotify_fd = inotify_init();
        if (inotify_fd < 0)
        {
            LOG_ERROR << "[HORIZON WALL] Failed to initialize inotify";
            return;
        }

        watch_fd = inotify_add_watch(inotify_fd, m_config_path.c_str(),
                                     IN_CLOSE_WRITE | IN_MOVED_TO);
        
        if (watch_fd < 0)
        {
            LOG_ERROR << "[HORIZON WALL] Failed to add watch for: " << m_config_path;
            return;
        }

        running = true;
        watcher_thread = std::thread(&WallApplication::watch_loop, this);
        LOG_INFO << "[HORIZON WALL] Started config watcher for: " << m_config_path;
    }

    void WallApplication::stop_watcher()
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

    void WallApplication::watch_loop()
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
                    
                    LOG_INFO << "[HORIZON WALL] Config change detected, reloading...";
                    m_window->post_task([this]() {
                        load_wallpaper(m_wall_path);
                    });
                }
            }
        }
    }
} // namespace horizon
