#include "ConfigReader.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace horizon {
namespace terminal {

TerminalConfig ConfigReader::load() {
    TerminalConfig config;
    const char* home = std::getenv("HOME");
    if (!home) return config;

    std::filesystem::path config_path(home);
    config_path /= ".config/horizon/horizon.json";

    if (!std::filesystem::exists(config_path)) return config;

    try {
        std::ifstream file(config_path);
        nlohmann::json data;
        file >> data;

        if (data.contains("terminal")) {
            auto& term_data = data["terminal"];
            if (term_data.contains("font")) {
                config.font = term_data["font"];
            }
            if (term_data.contains("font_size")) {
                config.font_size = term_data["font_size"];
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
        }
    } catch (const std::exception& e) {
        std::cerr << "[TerminalConfig] Error loading config: " << e.what() << std::endl;
    }

    return config;
}

} // namespace terminal
} // namespace horizon
