#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <horizon/Logger.hpp>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace horizon
{
    namespace terminal
    {

        struct ColorGroup
        {
            std::string black;
            std::string red;
            std::string green;
            std::string yellow;
            std::string blue;
            std::string magenta;
            std::string cyan;
            std::string white;

            std::vector<std::string> to_vector() const
            {
                return {black, red, green, yellow, blue, magenta, cyan, white};
            }
        };

        struct TerminalColorScheme
        {
            std::string name;
            struct
            {
                std::string background;
                std::string foreground;
                std::string cursor;
            } primary;
            ColorGroup normal;
            ColorGroup bright;

            static TerminalColorScheme default_theme()
            {
                TerminalColorScheme scheme;
                scheme.name = "Dracula";

                scheme.primary.background = "#282a36";
                scheme.primary.foreground = "#f8f8f2";
                scheme.primary.cursor = "#f8f8f2";

                scheme.normal.black = "#21222c";
                scheme.normal.red = "#ff5555";
                scheme.normal.green = "#50fa7b";
                scheme.normal.yellow = "#f1fa8c";
                scheme.normal.blue = "#bd93f9";
                scheme.normal.magenta = "#ff79c6";
                scheme.normal.cyan = "#8be9fd";
                scheme.normal.white = "#f8f8f2";

                scheme.bright.black = "#6272a4";
                scheme.bright.red = "#ff6e6e";
                scheme.bright.green = "#69ff94";
                scheme.bright.yellow = "#ffffa5";
                scheme.bright.blue = "#d6acff";
                scheme.bright.magenta = "#ff92df";
                scheme.bright.cyan = "#a4ffff";
                scheme.bright.white = "#ffffff";

                return scheme;
            }

            static TerminalColorScheme from_json(const nlohmann::json &j)
            {
                TerminalColorScheme scheme = default_theme();
                if (j.is_null())
                    return scheme;

                try
                {
                    scheme.name = j.value("name", scheme.name);

                    if (j.contains("colors"))
                    {
                        auto colors = j["colors"];

                        if (colors.contains("primary"))
                        {
                            auto primary = colors["primary"];
                            scheme.primary.background =
                                primary.value("background", scheme.primary.background);
                            scheme.primary.foreground =
                                primary.value("foreground", scheme.primary.foreground);
                            scheme.primary.cursor = primary.value(
                                "cursor", primary.value("foreground", scheme.primary.cursor));
                        }

                        if (colors.contains("normal"))
                        {
                            auto normal = colors["normal"];
                            scheme.normal.black = normal.value("black", scheme.normal.black);
                            scheme.normal.red = normal.value("red", scheme.normal.red);
                            scheme.normal.green = normal.value("green", scheme.normal.green);
                            scheme.normal.yellow = normal.value("yellow", scheme.normal.yellow);
                            scheme.normal.blue = normal.value("blue", scheme.normal.blue);
                            scheme.normal.magenta = normal.value("magenta", scheme.normal.magenta);
                            scheme.normal.cyan = normal.value("cyan", scheme.normal.cyan);
                            scheme.normal.white = normal.value("white", scheme.normal.white);
                        }

                        if (colors.contains("bright"))
                        {
                            auto bright = colors["bright"];
                            scheme.bright.black = bright.value("black", scheme.bright.black);
                            scheme.bright.red = bright.value("red", scheme.bright.red);
                            scheme.bright.green = bright.value("green", scheme.bright.green);
                            scheme.bright.yellow = bright.value("yellow", scheme.bright.yellow);
                            scheme.bright.blue = bright.value("blue", scheme.bright.blue);
                            scheme.bright.magenta = bright.value("magenta", scheme.bright.magenta);
                            scheme.bright.cyan = bright.value("cyan", scheme.bright.cyan);
                            scheme.bright.white = bright.value("white", scheme.bright.white);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "Error parsing color scheme JSON: " << e.what();
                }
                return scheme;
            }

            static TerminalColorScheme from_json(const std::string &path)
            {
                try
                {
                    std::ifstream f(path);
                    if (!f.is_open())
                    {
                        LOG_ERROR << "Could not open color scheme file: " << path;
                        return default_theme();
                    }

                    nlohmann::json j;
                    f >> j;
                    return from_json(j);
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "Error loading color scheme file: " << e.what() << " at path "
                              << path;
                    return default_theme();
                }
            }

            nlohmann::json to_json() const
            {
                nlohmann::json j;
                j["name"] = name;

                nlohmann::json colors;

                nlohmann::json j_primary;
                j_primary["background"] = this->primary.background;
                j_primary["foreground"] = this->primary.foreground;
                j_primary["cursor"] = this->primary.cursor;
                colors["primary"] = j_primary;

                auto serialize_group = [](const ColorGroup &group)
                {
                    nlohmann::json g;
                    g["black"] = group.black;
                    g["red"] = group.red;
                    g["green"] = group.green;
                    g["yellow"] = group.yellow;
                    g["blue"] = group.blue;
                    g["magenta"] = group.magenta;
                    g["cyan"] = group.cyan;
                    g["white"] = group.white;
                    return g;
                };

                colors["normal"] = serialize_group(this->normal);
                colors["bright"] = serialize_group(this->bright);

                j["colors"] = colors;
                return j;
            }

            static std::vector<std::string> get_theme_directories()
            {
                std::vector<std::string> dirs;

                // 1. User Themes
                char *home = std::getenv("HOME");
                if (home)
                {
                    std::filesystem::path p(home);
                    p /= ".local/share/horizon/terminal/color-schemes/";
                    dirs.push_back(p.string());
                }

                // 2. System Themes
                dirs.push_back("/usr/share/horizon/terminal/color-schemes/");

                // 3. Dev Themes (Fallback)
                dirs.push_back(
                    "/home/horacio/Desarrollo/austral-os/horizon/examples/usr/data/terminal/");

                return dirs;
            }

            static std::vector<TerminalColorScheme> list_available_themes()
            {
                std::map<std::string, TerminalColorScheme> themes_map;

                // Add default theme first
                TerminalColorScheme def = default_theme();
                std::string def_name = "System Default";
                def.name = def_name;
                themes_map[def_name] = def;

                auto dirs = get_theme_directories();

                // Scan in reverse order (Dev -> System -> User) so that later ones (higher
                // priority) overwrite earlier ones in the map
                std::reverse(dirs.begin(), dirs.end());

                for (const auto &dir_path : dirs)
                {
                    if (!std::filesystem::exists(dir_path))
                        continue;

                    try
                    {
                        for (const auto &entry : std::filesystem::directory_iterator(dir_path))
                        {
                            if (entry.path().extension() == ".json")
                            {
                                TerminalColorScheme theme = from_json(entry.path().string());
                                // The map key ensures we only have one theme with a given name
                                // User themes (last in scan but highest priority if we didn't
                                // reverse) Actually, map[name] = theme will overwrite. If we want
                                // User to win, we should scan System first, then User.
                                themes_map[theme.name] = theme;
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        LOG_ERROR << "Error scanning theme directory " << dir_path << ": "
                                  << e.what();
                    }
                }

                std::vector<TerminalColorScheme> result;
                // Move System Default to front if present
                if (themes_map.count(def_name))
                {
                    result.push_back(themes_map[def_name]);
                    themes_map.erase(def_name);
                }

                for (auto const &[name, theme] : themes_map)
                {
                    result.push_back(theme);
                }

                return result;
            }
        };

    } // namespace terminal
} // namespace horizon
