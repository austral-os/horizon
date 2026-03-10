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

    std::string DesktopEntry::get_icon_name(const std::string &app_id)
    {
        std::string path = find_desktop_file(app_id);
        if (path.empty())
            return "";

        LOG_INFO << "[DesktopEntry] Parsing file: " << path;
        std::ifstream file(path);
        if (!file.is_open())
            return "";

        std::string line;
        bool in_desktop_entry = false;

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

            if (line.rfind("Icon=", 0) == 0)
            {
                std::string icon = trim(line.substr(5));
                LOG_INFO << "[DesktopEntry] Found icon name: " << icon;
                return icon;
            }
        }

        LOG_INFO << "[DesktopEntry] No icon found in " << path;
        return "";
    }

    std::string DesktopEntry::find_desktop_file(const std::string &app_id)
    {
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
                    return full_path;
                }
            }
        }

        LOG_INFO << "[DesktopEntry] No desktop file found for app_id: " << app_id;
        return "";
    }

    std::vector<std::string> DesktopEntry::get_desktop_search_dirs()
    {
        std::vector<std::string> dirs;

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
