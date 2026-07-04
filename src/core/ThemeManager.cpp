#include "horizon/ThemeManager.hpp"
#include "horizon/EventsManager.hpp"

#include <cctype>
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

    static const char* fallback_color_scheme_json = R"({
    "colors": {
        "dark": {
            "air_border": "#1c1c1e",
            "air_default1": "#2c2c2e",
            "air_default2": "#3a3a3c",
            "aqua_default1": "#1c1c1e",
            "aqua_default2": "#2c2c2e",
            "aqua_highlight1": "#3a3a3c",
            "aqua_primary1": "#007aff",
            "aqua_primary2": "#0a84ff",
            "default1": "#1c1c1e",
            "default2": "#2c2c2e",
            "dock_glare": "#4a4a4c",
            "dock_lip1": "#3a3a3c",
            "dock_lip2": "#1c1c1e",
            "dock_shadow": "#000000",
            "dock_surface1": "#2c2c2e",
            "dock_surface2": "#1c1c1e",
            "error1": "#ff453a",
            "error2": "#ff6961",
            "group_alt_1": "#2c2c2e",
            "group_alt_2": "#3a3a3c",
            "group_separator": "#38383a",
            "icon_fg": "#98989d",
            "info1": "#0a84ff",
            "info2": "#64d2ff",
            "menu_bg": "#2c2c2e",
            "menu_bg1": "#1c1c1e",
            "menu_bg2": "#2c2c2e",
            "menu_border": "#3a3a3c",
            "menu_item_fg": "#ffffff",
            "menu_item_selected_bg1": "#007aff",
            "menu_item_selected_bg2": "#0056b3",
            "menu_item_selected_fg": "#ffffff",
            "menu_item_shortcut_fg": "#8e8e93",
            "menu_separator": "#38383a",
            "notification_bg": "#2c2c2e",
            "notification_fg": "#ffffff",
            "panel_bg1": "#1c1c1e",
            "panel_bg2": "#2c2c2e",
            "panel_border": "#3a3a3c",
            "primary1": "#007aff",
            "primary2": "#0a84ff",
            "ribbon_tab_active_title_bg": "#26262c",
            "ribbon_tab_title_bg": "#36363c",
            "secondary1": "#0a84ff",
            "secondary2": "#007aff",
            "sidebar_bg": "#36363c",
            "sidebar_border": "#3a3a3c",
            "sidebar_item_fg": "#98989d",
            "sidebar_item_fg2": "#ffffff",
            "solid_border": "#262626",
            "solid_default": "#3a3a3c",
            "solid_highlight": "#35353a",
            "solid_highlight2": "#636366",
            "solid_primary": "#007aff",
            "success1": "#34c759",
            "success2": "#30d158",
            "tab_button_1": "#1c1cae",
            "tab_button_2": "#2c2c2e",
            "tab_header_1": "#1c1c1e",
            "tab_header_2": "#2c2c2e",
            "table_row": "#1c1c1e",
            "table_row_alternate": "#2c2c2e",
            "table_row_fg": "#ffffff",
            "table_row_selected": "#007aff",
            "table_row_selected_fg": "#ffffff",
            "textbox_bg": "#1c1c1e",
            "textbox_brd": "#3a3a3c",
            "textbox_fg": "#ffffff",
            "textbox_focus": "#007aff",
            "textbox_invalid": "#ff453a",
            "textbox_ph_fg": "#8e8e93",
            "titlebar_bg1": "#1c1c1e",
            "titlebar_bg2": "#2c2c2e",
            "titlebar_border": "#3a3a3c",
            "titlebar_fg": "#ffffff",
            "warning1": "#ff9f0a",
            "warning2": "#ffb340",
            "window_bg": "#1c1c1e",
            "window_bg_alt": "#1f1f1f",
            "window_border": "#3a3a3c",
            "window_fg": "#dfdfdf"
        },
        "light": {
            "air_border": "#666666",
            "air_default1": "#ffffff",
            "air_default2": "#f5f5f5",
            "aqua_default1": "#616161",
            "aqua_default2": "#dddddd",
            "aqua_highlight1": "#f3f3f3",
            "aqua_primary1": "#1e4ce1",
            "aqua_primary2": "#61aff5",
            "default1": "#616161",
            "default2": "#dddddd",
            "dock_glare": "#ffffff",
            "dock_lip1": "#cccccc",
            "dock_lip2": "#666666",
            "dock_shadow": "#191919",
            "dock_surface1": "#ffffff",
            "dock_surface2": "#ffffff",
            "error1": "#e11e41",
            "error2": "#f5617f",
            "group_alt_1": "#f5f5f5",
            "group_alt_2": "#ffffff",
            "group_separator": "#cccccc",
            "icon_fg": "#616161",
            "info1": "#e1d31e",
            "info2": "#f5de61",
            "menu_bg": "#ffffff",
            "menu_bg1": "#ffffff",
            "menu_bg2": "#f1f1f1",
            "menu_border": "#b2b2b2",
            "menu_item_fg": "#000000",
            "menu_item_selected_bg1": "#3373e6",
            "menu_item_selected_bg2": "#1a59d9",
            "menu_item_selected_fg": "#ffffff",
            "menu_item_shortcut_fg": "#666666",
            "menu_separator": "#d9d9d9",
            "notification_bg": "#ffffff",
            "notification_fg": "#000000",
            "panel_bg1": "#e6e6e6",
            "panel_bg2": "#cccccc",
            "panel_border": "#b3b3b3",
            "primary1": "#1e4ce1",
            "primary2": "#61aff5",
            "ribbon_tab_active_title_bg": "#bdc6db",
            "ribbon_tab_title_bg": "#adb7cc",
            "secondary1": "#61aff5",
            "secondary2": "#1e4ce1",
            "sidebar_bg": "#dfe6ee",
            "sidebar_border": "#787878",
            "sidebar_item_fg": "#333333",
            "sidebar_item_fg2": "#ffffff",
            "solid_border": "#f3f3f3",
            "solid_default": "#ffffff",
            "solid_highlight": "#f3f3f3",
            "solid_highlight2": "#ffffff",
            "solid_primary": "#1e4ce1",
            "success1": "#1ee176",
            "success2": "#61f5a3",
            "tab_button_1": "#c2f2ff",
            "tab_button_2": "#b1b1c1",
            "tab_header_1": "#f2f2f2",
            "tab_header_2": "#d1d1d1",
            "table_row": "#ffffff",
            "table_row_alternate": "#f3f8ff",
            "table_row_fg": "#212121",
            "table_row_selected": "#1e4ce1",
            "table_row_selected_fg": "#ffffff",
            "textbox_bg": "#ffffff",
            "textbox_brd": "#666666",
            "textbox_fg": "#000000",
            "textbox_focus": "#66b3ff",
            "textbox_invalid": "#e11e41",
            "textbox_ph_fg": "#999999",
            "titlebar_bg1": "#d8d8d8",
            "titlebar_bg2": "#fafafa",
            "titlebar_border": "#787878",
            "titlebar_fg": "#212121",
            "warning1": "#e16b1e",
            "warning2": "#f59f61",
            "window_bg": "#f2f2f2",
            "window_bg_alt": "#f4f4f4",
            "window_border": "#787878",
            "window_fg": "#212121"
        }
    },
    "fonts": {
        "titlebar": {
            "family": "Lucida Grande",
            "size": 16,
            "weight": "bold"
        },
        "window": {
            "family": "Lucida Grande",
            "size": 20,
            "weight": "bold"
        }
    },
    "menu_opacity": 0.803398072719574,
    "panel_opacity": 0.6340579390525818,
    "variant": "light"
})";
    
    bool ThemeManager::load()
    {
        auto do_fallback = [this]() {
            try {
                json j = json::parse(fallback_color_scheme_json);
                this->parse_json(j);
                debug_log("fallback successfully loaded.");
            } catch (const std::exception &e) {
                debug_log(std::string("fallback failed: ") + e.what());
            }
        };

        std::lock_guard<std::recursive_mutex> lock(mutex);
        debug_log("load() called. config_path: " + config_path);

        std::ifstream file(config_path);
        if (!file.is_open())
        {
            debug_log("load() failed: could not open file: " + config_path + ". Loading fallback.");
            do_fallback();
            return false;
        }

        json j;

        try
        {
            file >> j;
            bool ok = parse_json(j);
            if (!ok) {
                debug_log("load() parse_json failed. Loading fallback.");
                do_fallback();
            }
            debug_log("load() parsed JSON success: " + std::string(ok ? "true" : "false") + ", variant: " + active_variant + ", opacity: " + std::to_string(panel_opacity));
            return ok;
        }
        catch (const std::exception &e)
        {
            debug_log("load() threw exception: " + std::string(e.what()) + ". Loading fallback.");
            do_fallback();
            return false;
        }
        catch (...)
        {
            debug_log("load() threw unknown exception! Loading fallback.");
            do_fallback();
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

    // Resolve the path to an app's color-scheme.json
    static std::string resolve_app_color_scheme_path_impl(const std::string &app_id)
    {
        // 1. Source tree (development)
#ifdef HORIZON_SOURCE_DIR
        {
            std::string p = std::string(HORIZON_SOURCE_DIR) + "/apps/" + app_id + "/assets/color-scheme.json";
            if (fs::exists(p))
                return p;
        }
#endif

        // 2. Installed system path
        {
            std::string p = "/usr/share/horizon/apps/" + app_id + "/color-scheme.json";
            if (fs::exists(p))
                return p;
        }

        // 3. Build directory (via HORIZON_BUILD_DIR)
#ifdef HORIZON_BUILD_DIR
        {
            std::string p = std::string(HORIZON_BUILD_DIR) + "/apps/" + app_id + "/assets/color-scheme.json";
            if (fs::exists(p))
                return p;
        }
#endif

        // 4. Executable directory (dev-copy from CMake POST_BUILD copies assets/ to $<TARGET_FILE_DIR>/assets/)
        {
            std::error_code ec;
            auto exe_path = fs::read_symlink("/proc/self/exe", ec);
            if (!ec)
            {
                std::string p = (exe_path.parent_path() / "assets" / "color-scheme.json").string();
                if (fs::exists(p, ec))
                    return p;
            }
        }

        // 5. CWD-relative (running from project root)
        {
            std::string p = "apps/" + app_id + "/assets/color-scheme.json";
            if (fs::exists(p))
                return p;
        }

        return "";
    }

    Color ThemeManager::get_color(const std::string &role) const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        // Check active app scheme first
        if (!m_active_app_id.empty())
        {
            auto app_it = m_app_schemes.find(m_active_app_id);
            if (app_it != m_app_schemes.end())
            {
                const auto &app_data = app_it->second;
                if (app_data.active_variant != "default")
                {
                    auto var_it = app_data.variants.find(app_data.active_variant);
                    if (var_it != app_data.variants.end())
                    {
                        auto role_it = var_it->second.find(role);
                        if (role_it != var_it->second.end())
                            return role_it->second;
                        // Role not in app variant — fall through to global
                    }
                }
            }
        }

        // Fall back to global scheme
        auto scheme_it = color_schemes.find(active_variant);
        if (scheme_it == color_schemes.end())
            return Color();

        auto role_it = scheme_it->second.find(role);
        if (role_it == scheme_it->second.end())
            return Color();

        return role_it->second;
    }

    bool ThemeManager::has_color(const std::string &role) const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        if (!m_active_app_id.empty())
        {
            auto app_it = m_app_schemes.find(m_active_app_id);
            if (app_it != m_app_schemes.end())
            {
                const auto &app_data = app_it->second;
                if (app_data.active_variant != "default")
                {
                    auto var_it = app_data.variants.find(app_data.active_variant);
                    if (var_it != app_data.variants.end() && var_it->second.find(role) != var_it->second.end())
                        return true;
                }
            }
        }

        auto scheme_it = color_schemes.find(active_variant);
        return scheme_it != color_schemes.end() && scheme_it->second.find(role) != scheme_it->second.end();
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

    bool ThemeManager::is_dark() const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return get_color("window_bg").is_dark();
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

    bool ThemeManager::load_app_color_scheme(const std::string &app_id)
    {
        std::string path = resolve_app_color_scheme_path_impl(app_id);
        debug_log("load_app_color_scheme(" + app_id + ") resolved to: " + (path.empty() ? "(not found)" : path));

        if (path.empty())
            return false;

        std::ifstream file(path);
        if (!file.is_open())
        {
            debug_log("load_app_color_scheme(" + app_id + ") — could not open file");
            return false;
        }

        json j;
        try
        {
            file >> j;
        }
        catch (const std::exception &e)
        {
            debug_log("load_app_color_scheme(" + app_id + ") — parse error: " + std::string(e.what()));
            return false;
        }

        if (!j.contains("colors") || !j.contains("variant"))
        {
            debug_log("load_app_color_scheme(" + app_id + ") — missing 'colors' or 'variant'");
            return false;
        }

        AppColorSchemeData data;

        if (j["colors"].is_object())
        {
            for (auto &[variant_name, variant_colors] : j["colors"].items())
            {
                if (!variant_colors.is_object())
                {
                    debug_log("load_app_color_scheme(" + app_id + ") — skipping non-object variant '" + variant_name + "'");
                    continue;
                }
                auto &target = data.variants[variant_name];
                for (auto &[role, hex_val] : variant_colors.items())
                {
                    if (!hex_val.is_string())
                    {
                        debug_log("load_app_color_scheme(" + app_id + ") — skipping non-string color '" + role + "'");
                        continue;
                    }
                    auto parsed = parse_hex(hex_val.get<std::string>());
                    if (parsed.has_value())
                    {
                        target[role] = parsed.value();
                    }
                    else
                    {
                        debug_log("load_app_color_scheme(" + app_id + ") — skipping color '" + role + "': invalid hex string '" + hex_val.get<std::string>() + "'");
                    }
                }
            }
        }
        else
        {
            debug_log("load_app_color_scheme(" + app_id + ") — 'colors' is not an object");
        }

        if (j["variant"].is_string())
        {
            data.default_variant = j["variant"].get<std::string>();
        }
        else
        {
            debug_log("load_app_color_scheme(" + app_id + ") — 'variant' is not a string, using 'light'");
            data.default_variant = "light";
        }
        data.active_variant = data.default_variant;

        size_t variant_count;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            m_app_schemes[app_id] = std::move(data);
            variant_count = m_app_schemes[app_id].variants.size();
        }

        debug_log("load_app_color_scheme(" + app_id + ") loaded " +
                  std::to_string(variant_count) +
                  " variants from " + path);
        return true;
    }

    bool ThemeManager::activate_app_color_scheme(const std::string &app_id)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        auto it = m_app_schemes.find(app_id);
        if (it == m_app_schemes.end())
        {
            debug_log("activate_app_color_scheme(" + app_id + ") — scheme not loaded");
            return false;
        }

        m_active_app_id = app_id;

        // Ensure active variant is initialized
        if (it->second.active_variant.empty())
        {
            it->second.active_variant = it->second.default_variant;
        }

        debug_log("activate_app_color_scheme(" + app_id + ") activated, variant=" + it->second.active_variant);

        ThemeEventContext ev;
        ev.sender = this;
        when_change.run(ev);

        return true;
    }

    void ThemeManager::deactivate_app_color_scheme()
    {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            if (m_active_app_id.empty())
                return;
            m_active_app_id.clear();
        }

        ThemeEventContext ev;
        ev.sender = this;
        when_change.run(ev);
    }

    std::vector<std::string> ThemeManager::app_color_scheme_variants(const std::string &app_id) const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        std::vector<std::string> result;
        result.push_back("default");

        auto it = m_app_schemes.find(app_id);
        if (it != m_app_schemes.end())
        {
            for (const auto &[name, _] : it->second.variants)
            {
                result.push_back(name);
            }
        }

        return result;
    }

    std::string ThemeManager::get_app_color_scheme_variant(const std::string &app_id) const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        auto it = m_app_schemes.find(app_id);
        if (it == m_app_schemes.end())
            return "default";

        return it->second.active_variant.empty() ? "default" : it->second.active_variant;
    }

    bool ThemeManager::set_app_color_scheme_variant(const std::string &app_id, const std::string &variant)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            auto it = m_app_schemes.find(app_id);
            if (it == m_app_schemes.end())
                return false;

            if (variant == "default")
            {
                it->second.active_variant = "default";
                debug_log("set_app_color_scheme_variant(" + app_id + ", " + variant + ")");
            }
            else
            {
                if (it->second.variants.find(variant) == it->second.variants.end())
                    return false;

                it->second.active_variant = variant;
                debug_log("set_app_color_scheme_variant(" + app_id + ", " + variant + ")");
            }
        }

        ThemeEventContext ev;
        ev.sender = this;
        when_change.run(ev);

        return true;
    }

    std::string ThemeManager::active_app_id() const
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return m_active_app_id;
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
                auto parsed = parse_hex(value.get<std::string>());
                if (parsed.has_value())
                {
                    color_schemes[scheme_name][role] = parsed.value();
                }
                else
                {
                    debug_log("parse_json() — invalid hex '" + value.get<std::string>() + "' for role '" + role + "', using black fallback");
                    color_schemes[scheme_name][role] = Color();
                }
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

    std::optional<Color> ThemeManager::parse_hex(const std::string &hex)
    {
        // Strict validation: only accept "#RRGGBB" format
        if (hex.size() != 7 || hex[0] != '#')
            return std::nullopt;

        for (size_t i = 1; i < 7; ++i)
        {
            if (!std::isxdigit(static_cast<unsigned char>(hex[i])))
                return std::nullopt;
        }

        unsigned int r = 0, g = 0, b = 0;

        int n = std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);

        if (n == 3)
        {
            Color col;
            col.r = static_cast<float>(r) / 255.0f;
            col.g = static_cast<float>(g) / 255.0f;
            col.b = static_cast<float>(b) / 255.0f;
            col.a = 1.0f;
            return col;
        }

        return std::nullopt;
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
