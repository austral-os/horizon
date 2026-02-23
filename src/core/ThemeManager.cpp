#include <cstdio>
#include <horizon/ThemeManager.hpp>
#include <sys/inotify.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sys/inotify.h>
#include <unistd.h>

using json = nlohmann::json;

namespace horizon
{
    static std::string get_config_path()
    {
        const char *home = getenv("HOME");
        return std::string(home) + "/.config/horizon/color-scheme.json";
    }

    ThemeManager::ThemeManager()
    {
        config_path = get_config_path();
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
        std::lock_guard<std::mutex> lock(mutex);

        std::ofstream file(config_path);
        if (!file.is_open())
            return false;

        file << to_json().dump(4);
        return true;
    }

    Color ThemeManager::get_color(const std::string &role) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = colors.find(role);
        if (it != colors.end())
            return it->second;

        Color col;
        col.r = 0;
        col.g = 0;
        col.b = 0;
        col.a = 255;

        return col;
    }

    void ThemeManager::set_color(const std::string &role, const Color &value)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            colors[role] = value;
        }

        save();

        if (on_theme_changed)
            on_theme_changed();
    }

    void ThemeManager::set_on_theme_changed(theme_changed_callback cb)
    {
        on_theme_changed = cb;
    }

    bool ThemeManager::parse_json(const json &j)
    {
        if (!j.contains("colors"))
            return false;

        colors.clear();

        for (auto &[key, value] : j["colors"].items())
        {
            colors[key] = parse_hex(value.get<std::string>());
        }

        if (on_theme_changed)
            on_theme_changed();

        return true;
    }

    json ThemeManager::to_json() const
    {
        json j;
        json color_object;

        for (const auto &[key, value] : colors)
        {
            color_object[key] = to_hex(value);
        }

        j["colors"] = color_object;
        return j;
    }

    Color ThemeManager::parse_hex(const std::string &hex)
    {
        unsigned int r, g, b;
        std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);

        Color col;
        col.r = static_cast<uint8_t>(r);
        col.g = static_cast<uint8_t>(g);
        col.b = static_cast<uint8_t>(b);
        col.a = 255;

        return col;
    }

    std::string ThemeManager::to_hex(const Color &c)
    {
        char buffer[8];
        std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", (int)c.r, (int)c.g, (int)c.b);
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

        while (running)
        {
            int length = read(inotify_fd, buffer, sizeof(buffer));

            if (length > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                load();
            }
        }
    }
} // namespace horizon
