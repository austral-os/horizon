#include <utils/WayfireConfigWriter.hpp>
#include <fstream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cstdlib>

namespace horizon::preferences
{
    void WayfireConfigWriter::update_monitor_config(const std::vector<MonitorConfig>& configs)
    {
        const char* home = std::getenv("HOME");
        if (!home) return;

        std::filesystem::path config_path(home);
        config_path /= ".config/wayfire.ini";

        std::vector<std::string> lines;
        if (std::filesystem::exists(config_path)) {
            std::ifstream file(config_path);
            std::string line;
            while (std::getline(file, line)) {
                lines.push_back(line);
            }
        }

        for (const auto& config : configs) {
            std::string section_name = "[output:" + config.name + "]";
            bool section_found = false;
            size_t section_start = 0;
            
            for (size_t i = 0; i < lines.size(); ++i) {
                std::string trimmed = lines[i];
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
                
                if (trimmed == section_name) {
                    section_found = true;
                    section_start = i;
                    break;
                }
            }
            
            std::vector<std::string> new_section_lines;
            if (config.enabled) {
                new_section_lines.push_back("mode=" + std::to_string(config.width) + "x" + std::to_string(config.height) + "@" + std::to_string((int)config.refresh));
                new_section_lines.push_back("position=" + std::to_string(config.x) + "," + std::to_string(config.y));
                std::string rot = "normal";
                if (config.rotation == 90) rot = "90";
                else if (config.rotation == 180) rot = "180";
                else if (config.rotation == 270) rot = "270";
                new_section_lines.push_back("transform=" + rot);
            } else {
                new_section_lines.push_back("mode=off");
            }
            
            if (section_found) {
                // Find end of section
                size_t section_end = section_start + 1;
                while (section_end < lines.size()) {
                    std::string next_trimmed = lines[section_end];
                    next_trimmed.erase(0, next_trimmed.find_first_not_of(" \t"));
                    next_trimmed.erase(next_trimmed.find_last_not_of(" \t") + 1);
                    if (!next_trimmed.empty() && next_trimmed[0] == '[' && next_trimmed.back() == ']') break;
                    section_end++;
                }
                
                // Replace old content with new content
                lines.erase(lines.begin() + section_start + 1, lines.begin() + section_end);
                lines.insert(lines.begin() + section_start + 1, new_section_lines.begin(), new_section_lines.end());
            } else {
                // Add new section
                lines.push_back("");
                lines.push_back(section_name);
                lines.insert(lines.end(), new_section_lines.begin(), new_section_lines.end());
            }
        }
        
        // Write back
        std::ofstream out(config_path);
        for (const auto& l : lines) {
            out << l << "\n";
        }
    }
}
