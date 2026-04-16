#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "TerminalColorScheme.hpp"

namespace horizon {
namespace terminal {

struct TerminalConfig {
    std::string font = "Monospace";
    int font_size = 12;
    int font_weight = 0;
    std::string cursor_style = "block";
    
    // Scrollbar settings
    bool show_scrollbar = false;
    int scrollback_lines = 5000;
    bool scroll_without_scrollbar = true;

    bool cursor_blink = true;
    TerminalColorScheme theme = TerminalColorScheme::default_theme();
};

class ConfigReader {
public:
    static TerminalConfig load();
    static std::string get_config_path();
};

} // namespace terminal
} // namespace horizon
