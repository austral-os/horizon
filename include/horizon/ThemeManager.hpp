#pragma once

#include "horizon/Color.hpp"
#include "horizon/EventsManager.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

        // --- App-local color scheme support ---

        /**
         * @brief Load an app-local color scheme from its assets/color-scheme.json.
         * Searches: source tree, installed path, build dir, cwd.
         * @return true if the file was found and parsed successfully.
         */
        bool load_app_color_scheme(const std::string &app_id);

        /**
         * @brief Activate an app as the current color scope.
         * When activated with a non-"default" variant, get_color() resolves
         * from the app scheme first, falling back to the global scheme.
         */
        bool activate_app_color_scheme(const std::string &app_id);

        /**
         * @brief Deactivate the current app color scope, reverting to global scheme only.
         */
        void deactivate_app_color_scheme();

        /**
         * @brief Returns "default" plus all variant names loaded for the given app.
         */
        std::vector<std::string> app_color_scheme_variants(const std::string &app_id) const;

        /**
         * @brief Get the currently active variant for an app (or "default").
         */
        std::string get_app_color_scheme_variant(const std::string &app_id) const;

        /**
         * @brief Set the active variant for an app. Validates existence ("default" always valid).
         * Fires when_change on success.
         */
        bool set_app_color_scheme_variant(const std::string &app_id, const std::string &variant);

        /**
         * @brief Get the currently active app id (empty if none).
         */
        std::string active_app_id() const;

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

        struct AppColorSchemeData
        {
            // variant_name -> (role -> color)
            std::unordered_map<std::string, std::unordered_map<std::string, Color>> variants;
            std::string default_variant;
            std::string active_variant; // "default" means use global scheme
        };

        mutable std::recursive_mutex mutex;

    private:
        void start_watcher();
        void stop_watcher();
        void watch_loop();

        bool parse_json(const nlohmann::json &j);
        nlohmann::json to_json() const;

        static std::optional<Color> parse_hex(const std::string &hex);
        static std::string to_hex(const Color &c);

        // app_id -> AppColorSchemeData
        std::unordered_map<std::string, AppColorSchemeData> m_app_schemes;
        std::string m_active_app_id; // currently active app scope, empty = global only
    };

    /**
     * @brief Global helper to access the system theme.
     */
    ThemeManager* theme_manager();

} // namespace horizon