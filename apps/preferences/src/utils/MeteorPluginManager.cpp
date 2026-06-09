#include <utils/MeteorPluginManager.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace horizon::preferences
{
    static std::string get_meteor_config_path()
    {
        const char* home = std::getenv("HOME");
        if (!home) return "";
        std::filesystem::path config_path(home);
        config_path /= ".config/meteor.ini";
        return config_path.string();
    }

    bool MeteorPluginManager::is_plugin_enabled(const std::string& plugin_name)
    {
        std::string path = get_meteor_config_path();
        if (path.empty() || !std::filesystem::exists(path)) return false;

        std::ifstream file(path);
        std::string line;
        bool in_core_section = false;

        while (std::getline(file, line))
        {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

            if (trimmed.empty()) continue;

            if (trimmed.front() == '[' && trimmed.back() == ']')
            {
                in_core_section = (trimmed == "[core]");
                continue;
            }

            if (in_core_section && trimmed.find("plugins") == 0)
            {
                size_t eq_pos = trimmed.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string plugins_str = trimmed.substr(eq_pos + 1);
                    std::stringstream ss(plugins_str);
                    std::string p;
                    while (ss >> p)
                    {
                        if (p == plugin_name) return true;
                    }
                }
            }
        }
        return false;
    }

    void MeteorPluginManager::set_plugin_enabled(const std::string& plugin_name, bool enable)
    {
        std::string path = get_meteor_config_path();
        if (path.empty()) return;

        std::vector<std::string> lines;
        bool core_section_exists = false;
        bool plugins_line_found = false;
        int core_section_index = -1;

        if (std::filesystem::exists(path))
        {
            std::ifstream file(path);
            std::string line;
            while (std::getline(file, line))
            {
                lines.push_back(line);
            }
        }

        bool in_core_section = false;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string trimmed = lines[i];
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

            if (trimmed.empty()) continue;

            if (trimmed.front() == '[' && trimmed.back() == ']')
            {
                in_core_section = (trimmed == "[core]");
                if (in_core_section) {
                    core_section_exists = true;
                    core_section_index = i;
                }
                continue;
            }

            if (in_core_section && trimmed.find("plugins") == 0)
            {
                size_t eq_pos = trimmed.find('=');
                if (eq_pos != std::string::npos)
                {
                    plugins_line_found = true;
                    std::string plugins_str = trimmed.substr(eq_pos + 1);
                    std::vector<std::string> plugins;
                    std::stringstream ss(plugins_str);
                    std::string p;
                    while (ss >> p)
                    {
                        if (p != plugin_name) plugins.push_back(p);
                    }

                    if (enable)
                    {
                        plugins.push_back(plugin_name);
                    }

                    std::string new_plugins_str = "plugins =";
                    for (const auto& pl : plugins)
                    {
                        new_plugins_str += " " + pl;
                    }
                    lines[i] = new_plugins_str;
                }
            }
        }

        if (!plugins_line_found)
        {
            std::string new_plugins_str = "plugins = " + plugin_name;
            if (!core_section_exists)
            {
                lines.push_back("");
                lines.push_back("[core]");
                lines.push_back(new_plugins_str);
            }
            else
            {
                lines.insert(lines.begin() + core_section_index + 1, new_plugins_str);
            }
        }

        std::ofstream out(path);
        for (const auto& l : lines)
        {
            out << l << "\n";
        }
    }
}
