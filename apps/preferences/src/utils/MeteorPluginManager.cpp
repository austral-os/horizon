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

    std::string MeteorPluginManager::get_config_value(const std::string& section, const std::string& key)
    {
        std::string path = get_meteor_config_path();
        if (path.empty() || !std::filesystem::exists(path)) return "";

        std::ifstream file(path);
        std::string line;
        bool in_target_section = false;
        std::string section_bracket = "[" + section + "]";

        while (std::getline(file, line))
        {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

            if (trimmed.empty()) continue;

            if (trimmed.front() == '[' && trimmed.back() == ']')
            {
                in_target_section = (trimmed == section_bracket);
                continue;
            }

            if (in_target_section && trimmed.find(key) == 0)
            {
                size_t eq_pos = trimmed.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key_part = trimmed.substr(0, eq_pos);
                    key_part.erase(key_part.find_last_not_of(" \t") + 1);
                    if (key_part == key)
                    {
                        std::string val_part = trimmed.substr(eq_pos + 1);
                        val_part.erase(0, val_part.find_first_not_of(" \t"));
                        return val_part;
                    }
                }
            }
        }
        return "";
    }

    void MeteorPluginManager::set_config_value(const std::string& section, const std::string& key, const std::string& value)
    {
        std::string path = get_meteor_config_path();
        if (path.empty()) return;

        std::vector<std::string> lines;
        bool section_exists = false;
        bool key_found = false;
        int section_index = -1;

        if (std::filesystem::exists(path))
        {
            std::ifstream file(path);
            std::string line;
            while (std::getline(file, line))
            {
                lines.push_back(line);
            }
        }

        bool in_target_section = false;
        std::string section_bracket = "[" + section + "]";

        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string trimmed = lines[i];
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

            if (trimmed.empty()) continue;

            if (trimmed.front() == '[' && trimmed.back() == ']')
            {
                in_target_section = (trimmed == section_bracket);
                if (in_target_section) {
                    section_exists = true;
                    section_index = i;
                }
                continue;
            }

            if (in_target_section && trimmed.find(key) == 0)
            {
                size_t eq_pos = trimmed.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key_part = trimmed.substr(0, eq_pos);
                    key_part.erase(key_part.find_last_not_of(" \t") + 1);
                    if (key_part == key)
                    {
                        key_found = true;
                        lines[i] = key + " = " + value;
                    }
                }
            }
        }

        if (!key_found)
        {
            std::string new_line = key + " = " + value;
            if (!section_exists)
            {
                lines.push_back("");
                lines.push_back(section_bracket);
                lines.push_back(new_line);
            }
            else
            {
                lines.insert(lines.begin() + section_index + 1, new_line);
            }
        }

        std::ofstream out(path);
        for (const auto& l : lines)
        {
            out << l << "\n";
        }
    }

    void MeteorPluginManager::remove_config_value(const std::string& section, const std::string& key)
    {
        std::string path = get_meteor_config_path();
        if (path.empty() || !std::filesystem::exists(path)) return;

        std::vector<std::string> lines;
        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }
        file.close();

        bool in_target_section = false;
        std::string section_bracket = "[" + section + "]";
        
        std::vector<std::string> new_lines;

        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string trimmed = lines[i];
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

            if (trimmed.empty()) {
                new_lines.push_back(lines[i]);
                continue;
            }

            if (trimmed.front() == '[' && trimmed.back() == ']')
            {
                in_target_section = (trimmed == section_bracket);
                new_lines.push_back(lines[i]);
                continue;
            }

            if (in_target_section && trimmed.find(key) == 0)
            {
                size_t eq_pos = trimmed.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key_part = trimmed.substr(0, eq_pos);
                    key_part.erase(key_part.find_last_not_of(" \t") + 1);
                    if (key_part == key)
                    {
                        // skip this line
                        continue;
                    }
                }
            }
            new_lines.push_back(lines[i]);
        }

        std::ofstream out(path);
        for (const auto& l : new_lines)
        {
            out << l << "\n";
        }
    }
}
