#include <utils/DesktopManager.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <map>

namespace fs = std::filesystem;

namespace horizon::preferences
{
    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::vector<DesktopEntry> DesktopManager::get_apps_for_mime(const std::string& mime_type)
    {
        MimeAssociations associations = load_mime_associations(mime_type);
        std::vector<DesktopEntry> all_entries = load_all_desktop_entries();
        
        std::vector<DesktopEntry> result;
        std::map<std::string, DesktopEntry> available_apps;
        for (const auto& entry : all_entries) {
            available_apps[entry.id] = entry;
        }

        auto is_removed = [&](const std::string& id) {
            return std::find(associations.removed_apps.begin(), associations.removed_apps.end(), id) != associations.removed_apps.end();
        };

        // 1. Add Default apps
        for (const auto& id : associations.default_apps) {
            if (available_apps.count(id) && !is_removed(id)) {
                result.push_back(available_apps[id]);
                available_apps.erase(id); // Prevent duplicates
            }
        }

        // 2. Add Added apps
        for (const auto& id : associations.added_apps) {
            if (available_apps.count(id) && !is_removed(id)) {
                result.push_back(available_apps[id]);
                available_apps.erase(id);
            }
        }

        // 3. Add remaining apps that support the MIME type
        for (auto it = available_apps.begin(); it != available_apps.end(); ) {
            const auto& entry = it->second;
            bool supports = false;
            for (const auto& m : entry.mime_types) {
                if (m == mime_type) {
                    supports = true;
                    break;
                }
            }
            
            if (supports && !is_removed(entry.id)) {
                result.push_back(entry);
                it = available_apps.erase(it);
            } else {
                ++it;
            }
        }

        return result;
    }

    std::vector<DesktopEntry> DesktopManager::load_all_desktop_entries()
    {
        std::vector<DesktopEntry> entries;
        std::vector<std::string> dirs = {
            "/usr/share/applications",
            "/usr/local/share/applications"
        };
        
        const char* home = std::getenv("HOME");
        if (home) {
            dirs.push_back(std::string(home) + "/.local/share/applications");
        }

        std::map<std::string, std::string> seen_ids; // id -> path (to handle overrides)

        for (const auto& dir : dirs) {
            if (!fs::exists(dir)) continue;
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
                    std::string id = entry.path().filename().string();
                    // Overrides: preferred order is local > usr/local > usr/share
                    // Since we iterate in order, we only add if not seen
                    if (seen_ids.count(id) == 0) {
                        auto desktop = parse_desktop_file(entry.path().string());
                        if (desktop) {
                            desktop->id = id;
                            entries.push_back(*desktop);
                            seen_ids[id] = entry.path().string();
                        }
                    }
                }
            }
        }
        return entries;
    }

    std::optional<DesktopEntry> DesktopManager::parse_desktop_file(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) return std::nullopt;

        DesktopEntry entry;
        entry.path = path;
        std::string line;
        bool in_main_section = false;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line == "[Desktop Entry]") {
                in_main_section = true;
                continue;
            } else if (line[0] == '[' && line != "[Desktop Entry]") {
                in_main_section = false;
                continue;
            }

            if (!in_main_section) continue;

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            if (key == "Name") entry.name = value;
            else if (key == "Icon") entry.icon = value;
            else if (key == "Exec") entry.exec = value;
            else if (key == "MimeType") {
                std::stringstream ss(value);
                std::string mime;
                while (std::getline(ss, mime, ';')) {
                    if (!mime.empty()) entry.mime_types.push_back(mime);
                }
            }
            else if (key == "NoDisplay" && value == "true") return std::nullopt;
        }

        if (entry.name.empty() || entry.exec.empty()) return std::nullopt;
        return entry;
    }

    DesktopManager::MimeAssociations DesktopManager::load_mime_associations(const std::string& mime_type)
    {
        MimeAssociations out;
        std::vector<std::string> config_paths;
        
        const char* home_config = std::getenv("XDG_CONFIG_HOME");
        const char* home = std::getenv("HOME");
        
        if (home_config) config_paths.push_back(std::string(home_config) + "/mimeapps.list");
        else if (home) config_paths.push_back(std::string(home) + "/.config/mimeapps.list");
        
        config_paths.push_back("/etc/xdg/mimeapps.list");
        config_paths.push_back("/usr/share/applications/mimeapps.list");

        for (const auto& path : config_paths) {
            parse_mimeapps_list(path, mime_type, out);
        }
        return out;
    }

    void DesktopManager::parse_mimeapps_list(const std::string& path, const std::string& mime_type, MimeAssociations& out)
    {
        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;
        std::string current_section;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line[0] == '[' && line.back() == ']') {
                current_section = line;
                continue;
            }

            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, pos));
            if (key != mime_type) continue;

            std::string values = trim(line.substr(pos + 1));
            std::stringstream ss(values);
            std::string id;
            while (std::getline(ss, id, ';')) {
                id = trim(id);
                if (id.empty()) continue;

                if (current_section == "[Default Applications]") {
                    if (std::find(out.default_apps.begin(), out.default_apps.end(), id) == out.default_apps.end())
                        out.default_apps.push_back(id);
                } else if (current_section == "[Added Associations]") {
                    if (std::find(out.added_apps.begin(), out.added_apps.end(), id) == out.added_apps.end())
                        out.added_apps.push_back(id);
                } else if (current_section == "[Removed Associations]") {
                    if (std::find(out.removed_apps.begin(), out.removed_apps.end(), id) == out.removed_apps.end())
                        out.removed_apps.push_back(id);
                }
            }
        }
    }

    void DesktopManager::add_mime_association(const std::string& mime_type, const std::string& desktop_id)
    {
        const char* home = std::getenv("HOME");
        if (!home) return;
        
        fs::path config_path = fs::path(home) / ".config" / "mimeapps.list";
        
        std::vector<std::string> lines;
        bool section_found = false;
        bool mime_found = false;
        
        if (fs::exists(config_path)) {
            std::ifstream infile(config_path);
            std::string line;
            while (std::getline(infile, line)) {
                lines.push_back(line);
            }
        } else {
            fs::create_directories(config_path.parent_path());
        }

        size_t section_idx = 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string l = trim(lines[i]);
            if (l == "[Added Associations]") {
                section_found = true;
                section_idx = i;
                break;
            }
        }

        if (!section_found) {
            lines.push_back("[Added Associations]");
            lines.push_back(mime_type + "=" + desktop_id + ";");
        } else {
            for (size_t i = section_idx + 1; i < lines.size(); ++i) {
                std::string l = trim(lines[i]);
                if (l.empty()) continue;
                if (l[0] == '[') {
                    lines.insert(lines.begin() + i, mime_type + "=" + desktop_id + ";");
                    mime_found = true;
                    break;
                }
                
                auto pos = l.find('=');
                if (pos != std::string::npos) {
                    std::string key = trim(l.substr(0, pos));
                    if (key == mime_type) {
                        mime_found = true;
                        std::string value = trim(l.substr(pos + 1));
                        if (value.find(desktop_id) == std::string::npos) {
                            if (!value.empty() && value.back() != ';') value += ";";
                            value += desktop_id + ";";
                            lines[i] = mime_type + "=" + value;
                        }
                        break;
                    }
                }
            }
            if (!mime_found) {
                lines.push_back(mime_type + "=" + desktop_id + ";");
            }
        }

        std::ofstream outfile(config_path);
        for (const auto& l : lines) {
            outfile << l << "\n";
        }
    }
} // namespace horizon::preferences
