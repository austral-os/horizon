#include "horizon/DesktopEntry.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <horizon/Logger.hpp>
#include <sstream>

namespace fs = std::filesystem;

namespace horizon
{
    static std::string trim(const std::string &s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        return s.substr(start, end - start + 1);
    }

    std::string DesktopEntry::get_value_from_desktop_file(const std::string &path,
                                                          const std::string &key)
    {
        if (path.empty())
            return "";

        std::ifstream file(path);
        if (!file.is_open())
            return "";

        std::string line;
        bool in_desktop_entry = false;
        std::string key_prefix = key + "=";

        while (std::getline(file, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;

            if (line[0] == '[' && line.back() == ']')
            {
                if (line == "[Desktop Entry]")
                {
                    in_desktop_entry = true;
                }
                else
                {
                    in_desktop_entry = false;
                }
                continue;
            }

            if (!in_desktop_entry)
                continue;

            if (line.rfind(key_prefix, 0) == 0)
            {
                return trim(line.substr(key_prefix.length()));
            }
        }
        return "";
    }

    std::map<std::string, std::string> DesktopEntry::s_icon_name_cache = {};
    std::map<std::string, std::string> DesktopEntry::s_desktop_file_cache = {};

    std::string DesktopEntry::get_icon_name(const std::string &app_id)
    {
        if (s_icon_name_cache.count(app_id))
            return s_icon_name_cache[app_id];

        std::string path = find_desktop_file(app_id);
        std::string icon = get_value_from_desktop_file(path, "Icon");
        if (!icon.empty())
        {
            LOG_INFO << "[DesktopEntry] Found icon name: " << icon << " for app_id: " << app_id;
        }
        s_icon_name_cache[app_id] = icon;
        return icon;
    }

    std::string DesktopEntry::get_exec_command(const std::string &app_id)
    {
        std::string path = find_desktop_file(app_id);
        return get_exec_command_from_path(path);
    }

    std::string DesktopEntry::get_exec_command_from_path(const std::string &path)
    {
        std::string exec = get_value_from_desktop_file(path, "Exec");
        if (!exec.empty())
        {
            LOG_INFO << "[DesktopEntry] Found exec command: " << exec << " in " << path;
        }
        return exec;
    }

    std::string DesktopEntry::find_desktop_file(const std::string &app_id)
    {
        if (s_desktop_file_cache.count(app_id))
            return s_desktop_file_cache[app_id];

        auto dirs = get_desktop_search_dirs();
        std::vector<std::string> candidates = {app_id + ".desktop"};

        // Some app_ids might have dots or be slightly different, but usually they match .desktop
        // filename
        for (const auto &dir : dirs)
        {
            if (!fs::is_directory(dir))
                continue;

            for (const auto &candidate : candidates)
            {
                std::string full_path = dir + "/" + candidate;
                if (fs::exists(full_path))
                {
                    LOG_INFO << "[DesktopEntry] Found candidate: " << full_path;
                    s_desktop_file_cache[app_id] = full_path;
                    return full_path;
                }
            }
        }

        LOG_INFO << "[DesktopEntry] No desktop file found for app_id: " << app_id;
        s_desktop_file_cache[app_id] = "";
        return "";
    }

    std::vector<std::string> DesktopEntry::s_additional_search_paths = {};

    void DesktopEntry::add_search_path(const std::string &path)
    {
        s_additional_search_paths.push_back(path);
    }

    void DesktopEntry::set_search_paths(const std::vector<std::string> &paths)
    {
        s_additional_search_paths = paths;
    }

    std::vector<std::string> DesktopEntry::get_desktop_search_dirs()
    {
        std::vector<std::string> dirs;

        // User specified paths first
        for (const auto &path : s_additional_search_paths)
        {
            dirs.push_back(path);
        }

        const char *home = std::getenv("HOME");
        if (home)
        {
            dirs.push_back(std::string(home) + "/.local/share/applications");
        }

        // Project local config
        dirs.push_back("/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps");

        const char *xdg_data = std::getenv("XDG_DATA_DIRS");
        if (xdg_data && xdg_data[0] != '\0')
        {
            std::istringstream ss(xdg_data);
            std::string path;
            while (std::getline(ss, path, ':'))
            {
                if (!path.empty())
                    dirs.push_back(path + "/applications");
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
