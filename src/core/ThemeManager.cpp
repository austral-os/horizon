#include "horizon/ThemeManager.hpp"
#include "horizon/EventsManager.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

using json = nlohmann::json;

namespace horizon
{
    static std::string get_user_config_path()
    {
        const char *home = std::getenv("HOME");
        if (!home)
            return "./color-scheme.json";

        return std::string(home) + "/.config/horizon/color-scheme.json";
    }

    static std::string get_system_config_path()
    {
        return "/usr/share/horizon/color-scheme.json";
    }

    static std::string get_active_config_path()
    {
        std::string user_path = get_user_config_path();
        if (fs::exists(user_path))
            return user_path;

        return get_system_config_path();
    }

    static void debug_log(const std::string &msg)
    {
        std::ofstream f("/tmp/theme_debug.log", std::ios::app);
        if (f.is_open())
        {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::string time_str = std::ctime(&time);
            if (!time_str.empty() && time_str.back() == '\n')
                time_str.pop_back();
            f << time_str << " [ThemeManager] " << msg << "\n";
        }
    }

    ThemeManager& ThemeManager::instance()
    {
        static ThemeManager inst;
        return inst;
    }

    ThemeManager* theme_manager()
    {
        return &ThemeManager::instance();
    }

    ThemeManager::ThemeManager()
    {
        config_path = get_active_config_path();
        debug_log("ThemeManager constructor. config_path resolved to: " + config_path);
        load();
        start_watcher();
    }

    ThemeManager::~ThemeManager()
    {
        debug_log("ThemeManager destructor.");
        stop_watcher();
    }

    bool ThemeManager::load()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        debug_log("load() called. config_path: " + config_path);

        std::ifstream file(config_path);
        if (!file.is_open())
        {
            debug_log("load() failed: could not open file: " + config_path);
            return false;
        }

        json j;

        try
        {
            file >> j;
            bool ok = parse_json(j);
            debug_log("load() parsed JSON success: " + std::string(ok ? "true" : "false") + ", variant: " + active_variant + ", opacity: " + std::to_string(panel_opacity));
            return ok;
        }
        catch (const std::exception &e)
        {
            debug_log("load() threw exception: " + std::string(e.what()));
            return false;
        }
        catch (...)
        {
            debug_log("load() threw unknown exception!");
            return false;
        }
    }

    bool ThemeManager::save()
    {
        std::string json_data;
        std::string user_path = get_user_config_path();

        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            json_data = to_json().dump(4);
        }
        
        // Ensure directory exists
        fs::path p(user_path);
        try {
            if (!fs::exists(p.parent_path())) {
                fs::create_directories(p.parent_path());
            }
        } catch (...) {
            return false;
        }

        std::ofstream file(user_path);
        if (!file.is_open())
            return false;

        file << json_data;
        file.close();
        
        // If we were using the system path, switch to the user path and restart watcher
        bool path_changed = false;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            if (config_path != user_path) {
                path_changed = true;
            }
        }

        if (path_changed) {
            stop_watcher();
            {
                std::lock_guard<std::recursive_mutex> lock(mutex);
                config_path = user_path;
            }
            start_watcher();
        }
        
        return true;
    }

    Color ThemeManager::get_color(const std::string &role) const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        auto scheme_it = color_schemes.find(active_variant);
        if (scheme_it == color_schemes.end())
            return Color();

        auto role_it = scheme_it->second.find(role);
        if (role_it == scheme_it->second.end())
            return Color();

        return role_it->second;
    }

    void ThemeManager::set_color(const std::string &role, const Color &value)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            color_schemes[active_variant][role] = value;
        }

        save();

        ThemeEventContext ev;
        ev.sender = this;

        when_change.run(ev);
    }

    std::string ThemeManager::get_variant() const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return active_variant;
    }

    void ThemeManager::set_variant(const std::string &variant_name)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            active_variant = variant_name;
        }

        save();

        ThemeEventContext ev;
        ev.sender = this;

        when_change.run(ev);
    }

    font_definition ThemeManager::get_font(const std::string &role) const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        auto it = fonts.find(role);
        if (it != fonts.end())
            return it->second;

        return font_definition{};
    }

    float ThemeManager::get_panel_opacity() const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return panel_opacity;
    }

    float ThemeManager::get_menu_opacity() const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return menu_opacity;
    }



    bool ThemeManager::parse_json(const json &j)
    {
        if (!j.contains("colors") || !j.contains("variant"))
            return false;

        color_schemes.clear();
        fonts.clear();

        // parse color schemes
        for (auto &[scheme_name, scheme_value] : j["colors"].items())
        {
            for (auto &[role, value] : scheme_value.items())
            {
                color_schemes[scheme_name][role] = parse_hex(value.get<std::string>());
            }
        }

        active_variant = j["variant"].get<std::string>();

        // parse fonts (optional)
        if (j.contains("fonts"))
        {
            for (auto &[role, font_json] : j["fonts"].items())
            {
                font_definition fd;

                if (font_json.contains("family"))
                    fd.family = font_json["family"].get<std::string>();

                if (font_json.contains("size"))
                    fd.size = font_json["size"].get<int>();

                if (font_json.contains("weight"))
                    fd.weight = font_json["weight"].get<std::string>();

                fonts[role] = fd;
            }
        }

        // parse opacities (optional)
        if (j.contains("panel_opacity") && j["panel_opacity"].is_number())
        {
            panel_opacity = j["panel_opacity"].get<float>();
        }
        else
        {
            panel_opacity = 1.0f;
        }

        if (j.contains("menu_opacity") && j["menu_opacity"].is_number())
        {
            menu_opacity = j["menu_opacity"].get<float>();
        }
        else
        {
            menu_opacity = 1.0f;
        }



        ThemeEventContext ev;
        ev.sender = this;

        when_change.run(ev);

        return true;
    }

    json ThemeManager::to_json() const
    {
        json j;

        json color_root;

        for (const auto &[scheme_name, scheme_map] : color_schemes)
        {
            json scheme_json;

            for (const auto &[role, color] : scheme_map)
            {
                scheme_json[role] = to_hex(color);
            }

            color_root[scheme_name] = scheme_json;
        }

        j["colors"] = color_root;
        j["variant"] = active_variant;

        json fonts_json;

        for (const auto &[role, font] : fonts)
        {
            fonts_json[role] = {
                {"family", font.family}, {"size", font.size}, {"weight", font.weight}};
        }

        j["fonts"] = fonts_json;

        j["panel_opacity"] = panel_opacity;
        j["menu_opacity"] = menu_opacity;

        return j;
    }

    Color ThemeManager::parse_hex(const std::string &hex)
    {
        unsigned int r = 0, g = 0, b = 0;

        std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);

        Color col;
        col.r = r / 255.0f;
        col.g = g / 255.0f;
        col.b = b / 255.0f;
        col.a = 1.0f;

        return col;
    }

    std::string ThemeManager::to_hex(const Color &c)
    {
        int r = static_cast<int>(c.r * 255.0f);
        int g = static_cast<int>(c.g * 255.0f);
        int b = static_cast<int>(c.b * 255.0f);

        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", r, g, b);

        return std::string(buffer);
    }

    void ThemeManager::start_watcher()
    {
        inotify_fd = inotify_init();
        if (inotify_fd < 0)
        {
            debug_log("start_watcher() failed to init inotify.");
            return;
        }

        // Get the active configuration path
        std::string active_path = get_active_config_path();
        
        // Resolve canonical path to support symlinks
        std::string real_path = active_path;
        try {
            if (fs::exists(active_path)) {
                real_path = fs::canonical(active_path).string();
            }
        } catch (...) {}

        std::string real_dir = fs::path(real_path).parent_path().string();
        std::string real_filename = fs::path(real_path).filename().string();

        debug_log("start_watcher() active_path: " + active_path + ", real_path: " + real_path + ", real_dir: " + real_dir + ", filename: " + real_filename);

        try {
            fs::create_directories(real_dir);
        } catch (...) {}

        // Watch the resolved canonical parent directory
        watch_fd = inotify_add_watch(inotify_fd, real_dir.c_str(),
                                     IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
        debug_log("start_watcher() added watch on: " + real_dir + " (watch_fd: " + std::to_string(watch_fd) + ")");

        // If we are currently not using the user config path directly, also watch the user config directory in case it gets created/modified later
        std::string user_path = get_user_config_path();
        std::string real_user_path = user_path;
        try {
            if (fs::exists(user_path)) {
                real_user_path = fs::canonical(user_path).string();
            }
        } catch (...) {}

        std::string real_user_dir = fs::path(real_user_path).parent_path().string();
        
        if (real_user_dir != real_dir) {
            try {
                fs::create_directories(real_user_dir);
            } catch (...) {}
            int user_wd = inotify_add_watch(inotify_fd, real_user_dir.c_str(),
                                            IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
            debug_log("start_watcher() also added watch on user_dir: " + real_user_dir + " (wd: " + std::to_string(user_wd) + ")");
        }

        running = true;

        watcher_thread = std::thread(&ThemeManager::watch_loop, this);
    }

    void ThemeManager::stop_watcher()
    {
        running = false;

        if (watcher_thread.joinable())
            watcher_thread.join();

        if (watch_fd >= 0)
            inotify_rm_watch(inotify_fd, watch_fd);

        if (inotify_fd >= 0)
            close(inotify_fd);
    }

    void ThemeManager::watch_loop()
    {
        char buffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
        struct pollfd pfd = {inotify_fd, POLLIN, 0};

        debug_log("watch_loop() started.");

        while (running)
        {
            int ret = poll(&pfd, 1, 100); // 100 ms timeout

            if (ret > 0 && (pfd.revents & POLLIN))
            {
                int length = read(inotify_fd, buffer, sizeof(buffer));

                if (length > 0)
                {
                    bool should_reload = false;

                    // Dynamically figure out target canonical filename
                    std::string target_filename = "color-scheme.json";
                    std::string active_path = get_active_config_path();
                    try {
                        if (fs::exists(active_path)) {
                            target_filename = fs::canonical(active_path).filename().string();
                        }
                    } catch (...) {}

                    for (char *ptr = buffer; ptr < buffer + length; )
                    {
                        struct inotify_event *event = reinterpret_cast<struct inotify_event *>(ptr);
                        if (event->len > 0)
                        {
                            std::string filename(event->name);
                            debug_log("watch_loop() inotify event on file: " + filename + ", mask: " + std::to_string(event->mask));
                            if (filename == "color-scheme.json" || filename == target_filename)
                            {
                                should_reload = true;
                            }
                        }
                        else
                        {
                            debug_log("watch_loop() direct inotify event, mask: " + std::to_string(event->mask));
                            should_reload = true;
                        }
                        ptr += sizeof(struct inotify_event) + event->len;
                    }

                    if (should_reload)
                    {
                        debug_log("watch_loop() triggering reload in 150ms...");
                        std::this_thread::sleep_for(std::chrono::milliseconds(150));

                        {
                            std::lock_guard<std::recursive_mutex> lock(mutex);
                            config_path = get_active_config_path();
                        }

                        debug_log("watch_loop() config_path updated to: " + config_path);
                        load();
                    }
                }
            }
        }
        debug_log("watch_loop() exited.");
    }
} // namespace horizon