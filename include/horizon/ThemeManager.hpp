#pragma once

#include "horizon/Color.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace horizon
{
    class ThemeManager
    {
    public:
        using theme_changed_callback = std::function<void()>;

        ThemeManager();
        ~ThemeManager();

        bool load();
        bool save();

        Color get_color(const std::string &role) const;
        void set_color(const std::string &role, const Color &value);

        void set_on_theme_changed(theme_changed_callback cb);

    private:
        std::string config_path;

        std::unordered_map<std::string, Color> colors;

        theme_changed_callback on_theme_changed;

        int inotify_fd = -1;
        int watch_fd = -1;

        std::thread watcher_thread;
        std::atomic<bool> running{false};

        mutable std::mutex mutex;

    private:
        void start_watcher();
        void stop_watcher();
        void watch_loop();

        bool parse_json(const nlohmann::json &j);
        nlohmann::json to_json() const;

        static Color parse_hex(const std::string &hex);
        static std::string to_hex(const Color &c);
    };
} // namespace horizon
