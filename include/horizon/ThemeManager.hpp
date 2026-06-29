#pragma once

#include "horizon/Color.hpp"
#include "horizon/EventsManager.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace horizon
{
    struct font_definition
    {
        std::string family;
        int size = 12;
        std::string weight;
    };

    class ThemeManager
    {
    public:
        using theme_changed_callback = std::function<void()>;

        static ThemeManager& instance();

        ~ThemeManager();

        bool load();
        bool save();

        Color get_color(const std::string &role) const;
        void set_color(const std::string &role, const Color &value);

        std::string get_variant() const;
        void set_variant(const std::string &variant_name);

        bool is_dark() const;

        font_definition get_font(const std::string &role) const;

        float get_panel_opacity() const;
        float get_menu_opacity() const;

        EventsManager<ThemeEventContext> when_change;

    private:
        ThemeManager();

        std::string config_path;

        // scheme_name -> (role -> color)
        std::unordered_map<std::string, std::unordered_map<std::string, Color>> color_schemes;

        // role -> font
        std::unordered_map<std::string, font_definition> fonts;

        std::string active_variant;

        float panel_opacity = 1.0f;
        float menu_opacity = 1.0f;

        int inotify_fd = -1;
        int watch_fd = -1;

        std::thread watcher_thread;
        std::atomic<bool> running{false};

        mutable std::recursive_mutex mutex;

    private:
        void start_watcher();
        void stop_watcher();
        void watch_loop();

        bool parse_json(const nlohmann::json &j);
        nlohmann::json to_json() const;

        static Color parse_hex(const std::string &hex);
        static std::string to_hex(const Color &c);
    };

    /**
     * @brief Global helper to access the system theme.
     */
    ThemeManager* theme_manager();

} // namespace horizon