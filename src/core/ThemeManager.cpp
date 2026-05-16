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
        load();
        start_watcher();
    }

    ThemeManager::~ThemeManager()
    {
        stop_watcher();
    }

    bool ThemeManager::load()
    {
        std::lock_guard<std::mutex> lock(mutex);

        std::ifstream file(config_path);
        if (!file.is_open())
            return false;

        json j;

        try
        {
            file >> j;
            return parse_json(j);
        }
        catch (...)
        {
            return false;
        }
    }

    bool ThemeManager::save()
    {
        std::string json_data;
        std::string user_path = get_user_config_path();

        {
            std::lock_guard<std::mutex> lock(mutex);
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
        // Note: we don't strictly need the mutex here for config_path if we assume
        // only one thread calls save/load, but it's safer to guard it if needed.
        // However, config_path is used by start_watcher/stop_watcher.
        
        if (config_path != user_path) {
            stop_watcher();
            config_path = user_path;
            start_watcher();
        }
        
        return true;
    }

    Color ThemeManager::get_color(const std::string &role) const
    {
        std::lock_guard<std::mutex> lock(mutex);

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
            std::lock_guard<std::mutex> lock(mutex);
            color_schemes[active_variant][role] = value;
        }

        save();

        ThemeEventContext ev;
        ev.sender = this;

        when_change.run(ev);
    }

    std::string ThemeManager::get_variant() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return active_variant;
    }

    void ThemeManager::set_variant(const std::string &variant_name)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            active_variant = variant_name;
        }

        save();

        ThemeEventContext ev;
        ev.sender = this;

        when_change.run(ev);
    }

    font_definition ThemeManager::get_font(const std::string &role) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = fonts.find(role);
        if (it != fonts.end())
            return it->second;

        return font_definition{};
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
            return;

        watch_fd = inotify_add_watch(inotify_fd, config_path.c_str(),
                                     IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE_SELF);

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
        char buffer[1024];
        struct pollfd pfd = {inotify_fd, POLLIN, 0};

        while (running)
        {
            int ret = poll(&pfd, 1, 100); // 100 ms timeout

            if (ret > 0 && (pfd.revents & POLLIN))
            {
                int length = read(inotify_fd, buffer, sizeof(buffer));

                if (length > 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));

                    load();
                }
            }
        }
    }
} // namespace horizon