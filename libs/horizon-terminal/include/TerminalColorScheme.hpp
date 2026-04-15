#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <horizon/Logger.hpp>

namespace horizon {
namespace terminal {

struct ColorGroup {
    std::string black;
    std::string red;
    std::string green;
    std::string yellow;
    std::string blue;
    std::string magenta;
    std::string cyan;
    std::string white;

    std::vector<std::string> to_vector() const {
        return {black, red, green, yellow, blue, magenta, cyan, white};
    }
};

struct TerminalColorScheme {
    std::string name;
    struct {
        std::string background;
        std::string foreground;
        std::string cursor;
    } primary;
    ColorGroup normal;
    ColorGroup bright;

    static TerminalColorScheme from_json(const std::string& path) {
        TerminalColorScheme scheme;
        try {
            std::ifstream f(path);
            if (!f.is_open()) {
                LOG_ERROR << "Could not open color scheme file: " << path;
                return scheme;
            }

            nlohmann::json j;
            f >> j;

            scheme.name = j.value("name", "Default");
            
            auto colors = j["colors"];
            auto primary = colors["primary"];
            scheme.primary.background = primary.value("background", "#000000");
            scheme.primary.foreground = primary.value("foreground", "#ffffff");
            scheme.primary.cursor = primary.value("cursor", scheme.primary.foreground);

            auto normal = colors["normal"];
            scheme.normal.black = normal.value("black", "#000000");
            scheme.normal.red = normal.value("red", "#ff0000");
            scheme.normal.green = normal.value("green", "#00ff00");
            scheme.normal.yellow = normal.value("yellow", "#ffff00");
            scheme.normal.blue = normal.value("blue", "#0000ff");
            scheme.normal.magenta = normal.value("magenta", "#ff00ff");
            scheme.normal.cyan = normal.value("cyan", "#00ffff");
            scheme.normal.white = normal.value("white", "#ffffff");

            auto bright = colors["bright"];
            scheme.bright.black = bright.value("black", "#555555");
            scheme.bright.red = bright.value("red", "#ff5555");
            scheme.bright.green = bright.value("green", "#55ff55");
            scheme.bright.yellow = bright.value("yellow", "#ffff55");
            scheme.bright.blue = bright.value("blue", "#5555ff");
            scheme.bright.magenta = bright.value("magenta", "#ff55ff");
            scheme.bright.cyan = bright.value("cyan", "#55ffff");
            scheme.bright.white = bright.value("white", "#ffffff");

        } catch (const std::exception& e) {
            LOG_ERROR << "Error parsing color scheme JSON: " << e.what();
        }
        return scheme;
    }
};

} // namespace terminal
} // namespace horizon
