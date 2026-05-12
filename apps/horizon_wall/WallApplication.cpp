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
#include <random>

namespace horizon
{
    WallApplication::WallApplication(const std::string &wall_path)
        : Application("org.horizon.wall", 1920, 1080, true, true),
          m_wall_path(wall_path)
    {
        const char* home = std::getenv("HOME");
        if (home)
        {
            m_config_path = std::string(home) + "/.config/horizon/desktop.json";
        }

        // Create the first window
        auto* first = create_layer_window("horizon_wall", 0, 0);
        m_windows.push_back(first);

        first->set_name("Horizon Wallpaper");
        first->set_icon_name("preferences-desktop-wallpaper");
        first->set_show_in_dock(false);

        // Setup About info
        auto &about = about_manager();
        about.set_app_title("Horizon Wallpaper");
        about.set_app_description("Horizon Wallpaper manages and displays your desktop backgrounds.");
        about.set_app_version(APP_VERSION);
        about.set_app_icon("preferences-desktop-wallpaper");
        about.set_app_git(ABOUT_HORIZON.git);
        setup_window(first);

        // When the first window is ready, check for more monitors
        first->add_on_start([this, first]() {
            int count = first->get_monitor_count();
            LOG_INFO << "[HORIZON WALL] Detected " << count << " monitors";
            for (int i = 1; i < count; ++i) {
                auto* win = create_layer_window("horizon_wall", 0, i);
                win->set_name("Horizon Wallpaper");
                win->set_icon_name("preferences-desktop-wallpaper");
                win->set_show_in_dock(false);
                setup_window(win);
                win->set_visible(true);
                m_windows.push_back(win);
            }
            
            // Trigger initial wallpaper load on the main thread loop (via first window's task queue)
            first->post_task([this]() {
                load_wallpaper(m_wall_path);
            });
        });

        start_watcher();

        first->set_visible(true);
        LOG_INFO << "Horizon Wallpaper initialized.";
    }

    WallApplication::~WallApplication()
    {
        stop_gallery();
        stop_watcher();
    }

    void WallApplication::setup_window(WaylandLayerWindow* window)
    {
        if (!window) return;
        window->set_anchor(
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        window->set_exclusive_zone(-1);
    }

    void WallApplication::load_wallpaper(const std::string &wall_path)
    {
        m_wallpaper_widgets.clear();
        ImageMode mode = ImageMode::Stretch;

        std::string final_path = wall_path;
        std::string type = "image";
        int change_time = 0;
        std::string order = "none";

        if (final_path.empty())
        {
            // Try to load from desktop.json
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
                        order = current.value("order", "none");
                        
                        if (fit == "fill") mode = ImageMode::Stretch;
                        else if (fit == "fit") mode = ImageMode::Fit;
                        else if (fit == "stretch") mode = ImageMode::Stretch;
                        else if (fit == "center") mode = ImageMode::Normal;
                        
                        LOG_INFO << "[HORIZON WALL] Loaded from config (desktop.json): " << final_path << " (fit: " << fit << ", type: " << type << ", order: " << order << ")";
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "[HORIZON WALL] Error parsing JSON: " << e.what();
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

        // Apply to all windows
        for (auto* win : m_windows)
        {
            auto root = std::make_unique<Widget>();
            root->set_layout_type(WIDGET_LAYOUT_VERTICAL);

            auto wallpaper = std::make_unique<Image>();
            wallpaper->set_mode(mode);
            wallpaper->when_mouse_press.connect(
                [](MouseButtonEventContext &ev)
                { LOG_INFO << "[HORIZON WALL] Wallpaper clicked with button: " << ev.button; });

            if (type != "gallery" && !final_path.empty())
            {
                wallpaper->set_path(final_path);
            }

            m_wallpaper_widgets.push_back(wallpaper.get());
            root->add_child(std::move(wallpaper));
            win->set_root(std::move(root));
        }

        if (type == "gallery" && !final_path.empty())
        {
            if (std::filesystem::is_directory(final_path))
            {
                start_gallery(final_path, change_time * 1000, order);
            }
            else if (std::filesystem::exists(final_path))
            {
                start_gallery(std::filesystem::path(final_path).parent_path().string(), change_time * 1000, order);
            }
        }
        else
        {
            stop_gallery();
        }
    }

    void WallApplication::start_gallery(const std::string &directory, int interval_ms, const std::string &order)
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

        if (order == "random")
        {
            LOG_INFO << "[HORIZON WALL] Shuffling gallery images";
            auto rd = std::random_device {}; 
            auto rng = std::default_random_engine { rd() };
            std::shuffle(m_gallery_images.begin(), m_gallery_images.end(), rng);
        }
        else
        {
            std::sort(m_gallery_images.begin(), m_gallery_images.end());
        }

        LOG_INFO << "[HORIZON WALL] Starting gallery with " << m_gallery_images.size() << " images, interval: " << interval_ms << "ms, order: " << order;
        
        // Show the first image immediately
        for (auto* widget : m_wallpaper_widgets)
        {
            widget->set_path(m_gallery_images[0]);
        }

        if (interval_ms > 0 && !m_windows.empty())
        {
            m_gallery_timer_id = m_windows[0]->add_timer(interval_ms, [this]() {
                next_gallery_image();
            }, true);
        }
    }

    void WallApplication::stop_gallery()
    {
        if (m_gallery_timer_id != 0 && !m_windows.empty())
        {
            m_windows[0]->stop_timer(m_gallery_timer_id);
            m_gallery_timer_id = 0;
        }
    }

    void WallApplication::next_gallery_image()
    {
        if (m_gallery_images.empty()) return;

        m_current_gallery_index = (m_current_gallery_index + 1) % m_gallery_images.size();
        
        LOG_INFO << "[HORIZON WALL] Switching to next gallery image: " << m_gallery_images[m_current_gallery_index];
        for (size_t i = 0; i < m_wallpaper_widgets.size(); ++i)
        {
            m_wallpaper_widgets[i]->set_path(m_gallery_images[m_current_gallery_index]);
            if (i < m_windows.size()) m_windows[i]->invalidate();
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
                    if (!m_windows.empty())
                    {
                        m_windows[0]->post_task([this]() {
                            load_wallpaper(m_wall_path);
                        });
                    }
                }
            }
        }
    }
} // namespace horizon
