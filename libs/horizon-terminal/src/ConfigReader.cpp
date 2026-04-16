#include "ConfigReader.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace horizon {
namespace terminal {

TerminalConfig ConfigReader::load() {
    TerminalConfig config;
    std::string config_path = get_config_path();
    if (config_path.empty() || !std::filesystem::exists(config_path)) return config;

    try {
        std::ifstream file(config_path);
        nlohmann::json data;
        file >> data;

        nlohmann::json term_data;
        if (data.contains("terminal")) {
            term_data = data["terminal"];
        } else {
            term_data = data; // Assume the root object contains terminal settings
        }

        // Look for theme at root level first (prioritize root as requested)
        if (data.contains("theme")) {
            config.theme = TerminalColorScheme::from_json(data["theme"]);
        } 
        // Fallback: look inside terminal section if root doesn't have it
        else if (term_data.contains("theme")) {
            config.theme = TerminalColorScheme::from_json(term_data["theme"]);
        } 
        // Final fallback: default Dracula theme
        else {
            config.theme = TerminalColorScheme::default_theme();
        }

        if (term_data.contains("font")) {
            config.font = term_data["font"];
        }
        if (term_data.contains("font_size")) {
            config.font_size = term_data["font_size"];
        }
        if (term_data.contains("font_weight")) {
            config.font_weight = term_data["font_weight"];
        }
        if (term_data.contains("cursor_style")) {
            config.cursor_style = term_data["cursor_style"];
        }
        
        if (term_data.contains("show_scrollbar")) {
            config.show_scrollbar = term_data["show_scrollbar"];
        }
        if (term_data.contains("scrollback_lines")) {
            config.scrollback_lines = term_data["scrollback_lines"];
        }
        if (term_data.contains("scroll_without_scrollbar")) {
            config.scroll_without_scrollbar = term_data["scroll_without_scrollbar"];
        }
        if (term_data.contains("cursor_blink")) {
            config.cursor_blink = term_data["cursor_blink"];
        }
        if (term_data.contains("transparency")) {
            config.transparency = term_data["transparency"];
        }
    } catch (const std::exception& e) {
        std::cerr << "[TerminalConfig] Error loading config: " << e.what() << std::endl;
    }

    return config;
}

std::string ConfigReader::get_config_path() {
    const char* home = std::getenv("HOME");
    if (!home) return "";

    std::filesystem::path config_path(home);
    config_path /= ".config/horizon/terminal.json";
    return config_path.string();
}

} // namespace terminal
} // namespace horizon
