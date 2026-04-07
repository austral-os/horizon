#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace horizon {
namespace terminal {

struct TerminalConfig {
    std::string font = "Monospace";
    int font_size = 12;
    std::string cursor_style = "block";
};

class ConfigReader {
public:
    static TerminalConfig load();
};

} // namespace terminal
} // namespace horizon
