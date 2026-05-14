#include "horizon/DesktopManager.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <map>
#include <cstdlib>
#include <horizon/Logger.hpp>

namespace fs = std::filesystem;

namespace horizon
{
    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string run_command_capture_output(const std::string &cmd)
    {
        char buffer[128];
        std::string result = "";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) result += buffer;
        int exit_code = pclose(pipe);
        if (exit_code != 0) return "";
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
        return result;
    }

    std::vector<std::string> DesktopManager::s_additional_search_paths = {};
    std::map<std::string, std::string> DesktopManager::s_desktop_file_cache = {};
    std::map<std::string, std::string> DesktopManager::s_icon_name_cache = {};
    std::vector<DesktopEntry> DesktopManager::s_all_apps_cache = {};
    std::map<std::string, std::string> DesktopManager::s_mime_type_cache = {};

    std::string DesktopManager::get_mime_type(const std::string& path)
    {
        if (s_mime_type_cache.count(path)) return s_mime_type_cache[path];

        // Fast path for extensions
        size_t dot = path.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = path.substr(dot + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            static const std::map<std::string, std::string> ext_map = {
                {"txt", "text/plain"},
                {"pdf", "application/pdf"},
                {"png", "image/png"},
                {"jpg", "image/jpeg"},
                {"jpeg", "image/jpeg"},
                {"gif", "image/gif"},
                {"svg", "image/svg+xml"},
                {"zip", "application/zip"},
                {"tar", "application/x-tar"},
                {"gz", "application/gzip"},
                {"7z", "application/x-7z-compressed"},
                {"mp3", "audio/mpeg"},
                {"mp4", "video/mp4"},
                {"html", "text/html"},
                {"cpp", "text/x-c++src"},
                {"hpp", "text/x-c++hdr"},
                {"c", "text/x-csrc"},
                {"h", "text/x-chdr"}
            };
            
            if (ext_map.count(ext)) {
                s_mime_type_cache[path] = ext_map.at(ext);
                return ext_map.at(ext);
            }
        }

        std::string result = run_command_capture_output("xdg-mime query filetype \"" + path + "\"");
        s_mime_type_cache[path] = result;
        return result;
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
        if (!s_all_apps_cache.empty()) return s_all_apps_cache;

        std::vector<DesktopEntry> entries;
        std::vector<std::string> dirs = get_desktop_search_dirs();
        
        std::map<std::string, std::string> seen_ids; // id -> path (to handle overrides)

        for (const auto& dir : dirs) {
            if (!fs::exists(dir)) continue;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
                        std::string id = entry.path().filename().string();
                        // Overrides: preferred order is local > usr/local > usr/share
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
            } catch (...) {
            }
        }
        s_all_apps_cache = entries;
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

    void DesktopManager::remove_mime_association(const std::string& mime_type, const std::string& desktop_id)
    {
        const char* home = std::getenv("HOME");
        if (!home) return;
        
        fs::path config_path = fs::path(home) / ".config" / "mimeapps.list";
        if (!fs::exists(config_path)) return;

        std::vector<std::string> lines;
        std::ifstream infile(config_path);
        std::string line;
        while (std::getline(infile, line)) {
            lines.push_back(line);
        }

        bool in_added_section = false;
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string l = trim(lines[i]);
            if (l == "[Added Associations]") {
                in_added_section = true;
                continue;
            } else if (l.size() > 0 && l[0] == '[') {
                in_added_section = false;
                continue;
            }

            if (in_added_section) {
                auto pos = l.find('=');
                if (pos != std::string::npos) {
                    std::string key = trim(l.substr(0, pos));
                    if (key == mime_type) {
                        std::string value = trim(l.substr(pos + 1));
                        
                        std::string search_str = desktop_id + ";";
                        size_t start_pos = value.find(search_str);
                        if (start_pos != std::string::npos) {
                            value.erase(start_pos, search_str.length());
                            if (value.empty()) {
                                lines.erase(lines.begin() + i);
                            } else {
                                lines[i] = mime_type + "=" + value;
                            }
                        }
                        break;
                    }
                }
            }
        }

        std::ofstream outfile(config_path);
        for (const auto& l : lines) {
            outfile << l << "\n";
        }
    }

    void DesktopManager::set_default_application(const std::string& mime_type, const std::string& desktop_id)
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
            if (l == "[Default Applications]") {
                section_found = true;
                section_idx = i;
                break;
            }
        }

        if (!section_found) {
            lines.push_back("[Default Applications]");
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
                        lines[i] = mime_type + "=" + desktop_id + ";";
                        break;
                    }
                }
            }
            if (!mime_found) {
                lines.insert(lines.begin() + section_idx + 1, mime_type + "=" + desktop_id + ";");
            }
        }

        std::ofstream outfile(config_path);
        for (const auto& l : lines) {
            outfile << l << "\n";
        }
    }

    std::vector<DesktopEntry> DesktopManager::load_autostart_entries()
    {
        std::vector<DesktopEntry> entries;
        const char* home = std::getenv("HOME");
        if (!home) return entries;

        fs::path autostart_dir = fs::path(home) / ".config" / "autostart";
        if (!fs::exists(autostart_dir)) return entries;

        for (const auto& entry : fs::directory_iterator(autostart_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".desktop") {
                auto desktop = parse_desktop_file(entry.path().string());
                if (desktop) {
                    desktop->id = entry.path().filename().string();
                    entries.push_back(*desktop);
                }
            }
        }
        return entries;
    }

    void DesktopManager::add_to_autostart(const DesktopEntry& entry)
    {
        const char* home = std::getenv("HOME");
        if (!home) return;

        fs::path autostart_dir = fs::path(home) / ".config" / "autostart";
        if (!fs::exists(autostart_dir)) {
            fs::create_directories(autostart_dir);
        }

        fs::path target_path = autostart_dir / entry.id;
        
        try {
            if (fs::exists(entry.path)) {
                fs::copy_file(entry.path, target_path, fs::copy_options::overwrite_existing);
            } else {
                std::ofstream outfile(target_path);
                outfile << "[Desktop Entry]\n";
                outfile << "Type=Application\n";
                outfile << "Name=" << entry.name << "\n";
                outfile << "Exec=" << entry.exec << "\n";
                if (!entry.icon.empty()) outfile << "Icon=" << entry.icon << "\n";
            }
        } catch (...) {
        }
    }

    void DesktopManager::update_autostart_cmd(const std::string& path, const std::string& new_cmd)
    {
        if (!fs::exists(path)) return;

        std::vector<std::string> lines;
        std::ifstream infile(path);
        std::string line;
        while (std::getline(infile, line)) {
            lines.push_back(line);
        }
        infile.close();

        bool found = false;
        for (auto& l : lines) {
            std::string trimmed = trim(l);
            if (trimmed.compare(0, 5, "Exec=") == 0) {
                l = "Exec=" + new_cmd;
                found = true;
                break;
            }
        }

        if (!found) {
            lines.push_back("Exec=" + new_cmd);
        }

        std::ofstream outfile(path);
        for (const auto& l : lines) {
            outfile << l << "\n";
        }
    }

    void DesktopManager::remove_from_autostart(const std::string& path)
    {
        if (fs::exists(path)) {
            fs::remove(path);
        }
    }

    // --- Utility methods moved from DesktopEntry ---

    std::string DesktopManager::get_value_from_desktop_file(const std::string &path, const std::string &key)
    {
        if (path.empty()) return "";

        std::ifstream file(path);
        if (!file.is_open()) return "";

        std::string line;
        bool in_desktop_entry = false;
        std::string key_prefix = key + "=";

        while (std::getline(file, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line[0] == '[' && line.back() == ']')
            {
                if (line == "[Desktop Entry]") in_desktop_entry = true;
                else in_desktop_entry = false;
                continue;
            }

            if (!in_desktop_entry) continue;

            if (line.rfind(key_prefix, 0) == 0)
            {
                return trim(line.substr(key_prefix.length()));
            }
        }
        return "";
    }

    std::string DesktopManager::get_icon_name(const std::string &app_id)
    {
        if (s_icon_name_cache.count(app_id)) return s_icon_name_cache[app_id];

        std::string path = find_desktop_file(app_id);
        std::string icon = get_value_from_desktop_file(path, "Icon");
        s_icon_name_cache[app_id] = icon;
        return icon;
    }

    std::string DesktopManager::get_exec_command(const std::string &app_id)
    {
        std::string path = find_desktop_file(app_id);
        return get_exec_command_from_path(path);
    }

    std::string DesktopManager::get_exec_command_from_path(const std::string &path)
    {
        return get_value_from_desktop_file(path, "Exec");
    }

    std::string DesktopManager::find_desktop_file(const std::string &app_id)
    {
        if (s_desktop_file_cache.count(app_id)) return s_desktop_file_cache[app_id];

        auto dirs = get_desktop_search_dirs();
        std::vector<std::string> candidates;
        
        if (app_id.size() >= 8 && app_id.substr(app_id.size() - 8) == ".desktop") candidates.push_back(app_id);
        else candidates.push_back(app_id + ".desktop");

        if (app_id.find('.') != std::string::npos)
        {
            std::string id_without_ext = app_id;
            if (app_id.size() >= 8 && app_id.substr(app_id.size() - 8) == ".desktop")
                id_without_ext = app_id.substr(0, app_id.size() - 8);

            size_t last_dot = id_without_ext.find_last_of('.');
            if (last_dot != std::string::npos)
            {
                std::string short_id = id_without_ext.substr(last_dot + 1);
                if (!short_id.empty()) candidates.push_back(short_id + ".desktop");
            }
        }

        for (const auto &dir : dirs)
        {
            if (!fs::is_directory(dir)) continue;
            for (const auto &candidate : candidates)
            {
                std::string full_path = dir + "/" + candidate;
                if (fs::exists(full_path))
                {
                    s_desktop_file_cache[app_id] = full_path;
                    return full_path;
                }
            }
        }

        s_desktop_file_cache[app_id] = "";
        return "";
    }

    void DesktopManager::add_search_path(const std::string &path)
    {
        s_additional_search_paths.push_back(path);
    }

    void DesktopManager::set_search_paths(const std::vector<std::string> &paths)
    {
        s_additional_search_paths = paths;
    }

    std::vector<std::string> DesktopManager::get_desktop_search_dirs()
    {
        std::vector<std::string> dirs;
        for (const auto &path : s_additional_search_paths) dirs.push_back(path);

        const char *home = std::getenv("HOME");
        if (home) dirs.push_back(std::string(home) + "/.local/share/applications");

        const char *xdg_data = std::getenv("XDG_DATA_DIRS");
        if (xdg_data && xdg_data[0] != '\0')
        {
            std::istringstream ss(xdg_data);
            std::string path;
            while (std::getline(ss, path, ':'))
            {
                if (!path.empty()) dirs.push_back(path + "/applications");
            }
        }
        else
        {
            dirs.push_back("/usr/local/share/applications");
            dirs.push_back("/usr/share/applications");
        }
        return dirs;
    }

} // namespace horizon
