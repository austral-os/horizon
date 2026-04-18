#include "horizon/IconThemeLookup.hpp"
#include "horizon/DesktopEntry.hpp"
#include <horizon/Logger.hpp>

#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

namespace horizon
{
    std::map<std::string, IconThemeLookup::ThemeInfo> IconThemeLookup::s_theme_cache;
    std::map<std::string, std::string> IconThemeLookup::s_resolution_cache;

    // ---------------------------------------------------------------------------
    // Utility: trim whitespace
    // ---------------------------------------------------------------------------
    static std::string trim(const std::string &s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        return s.substr(start, end - start + 1);
    }

    // ---------------------------------------------------------------------------
    // get_active_theme_name — reads the GTK3 settings.ini
    // ---------------------------------------------------------------------------
    std::string IconThemeLookup::get_active_theme_name()
    {
        const char *home = std::getenv("HOME");
        if (!home)
            return "hicolor";

        // Try GTK3 settings first
        std::string gtk3_path = std::string(home) + "/.config/gtk-3.0/settings.ini";
        std::ifstream gtk3(gtk3_path);
        if (gtk3.is_open())
        {
            std::string line;
            while (std::getline(gtk3, line))
            {
                line = trim(line);
                if (line.rfind("gtk-icon-theme-name", 0) == 0)
                {
                    auto eq = line.find('=');
                    if (eq != std::string::npos)
                    {
                        std::string val = trim(line.substr(eq + 1));
                        if (!val.empty())
                            return val;
                    }
                }
            }
        }

        // Try GTK4 settings
        std::string gtk4_path = std::string(home) + "/.config/gtk-4.0/settings.ini";
        std::ifstream gtk4(gtk4_path);
        if (gtk4.is_open())
        {
            std::string line;
            while (std::getline(gtk4, line))
            {
                line = trim(line);
                if (line.rfind("gtk-icon-theme-name", 0) == 0)
                {
                    auto eq = line.find('=');
                    if (eq != std::string::npos)
                    {
                        std::string val = trim(line.substr(eq + 1));
                        if (!val.empty())
                            return val;
                    }
                }
            }
        }

        return "hicolor";
    }

    // ---------------------------------------------------------------------------
    // get_base_dirs — XDG icon base directories
    // ---------------------------------------------------------------------------
    std::vector<std::string> IconThemeLookup::get_base_dirs()
    {
        std::vector<std::string> dirs;

        const char *home = std::getenv("HOME");
        if (home)
        {
            dirs.push_back(std::string(home) + "/.local/share/icons");
            dirs.push_back(std::string(home) + "/.icons");
        }

        // XDG_DATA_DIRS
        const char *xdg_data = std::getenv("XDG_DATA_DIRS");
        if (xdg_data && xdg_data[0] != '\0')
        {
            std::istringstream ss(xdg_data);
            std::string path;
            while (std::getline(ss, path, ':'))
            {
                if (!path.empty())
                    dirs.push_back(path + "/icons");
            }
        }
        else
        {
            dirs.push_back("/usr/local/share/icons");
            dirs.push_back("/usr/share/icons");
        }

#ifdef HORIZON_BUILD_DIR
        // Development mode: check extracted icons in build directory
        std::string dev_icons = std::string(HORIZON_BUILD_DIR) + "/apps/horizon_session/icons_extracted/austral-icon-theme/Light";
        if (fs::exists(dev_icons))
        {
            // We give it a high priority by adding it near the beginning or as its own entry
            // but we need to ensure the parent directory is treat as a "theme base dir"
            dirs.push_back(std::string(HORIZON_BUILD_DIR) + "/apps/horizon_session/icons_extracted/austral-icon-theme");
        }
#endif

        return dirs;
    }

    // ---------------------------------------------------------------------------
    // parse_index_theme — parse an index.theme file
    // ---------------------------------------------------------------------------
    IconThemeLookup::ThemeInfo IconThemeLookup::parse_index_theme(const std::string &theme_dir)
    {
        if (s_theme_cache.count(theme_dir))
            return s_theme_cache[theme_dir];

        ThemeInfo info;
        std::string index_path = theme_dir + "/index.theme";

        std::ifstream file(index_path);
        if (!file.is_open())
            return info;

        std::string current_section;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
        std::vector<std::string> directory_list;

        std::string line;
        while (std::getline(file, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#')
                continue;

            if (line[0] == '[' && line.back() == ']')
            {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }

            auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;

            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            if (current_section == "Icon Theme")
            {
                if (key == "Inherits")
                {
                    std::istringstream ss(value);
                    std::string parent;
                    while (std::getline(ss, parent, ','))
                    {
                        parent = trim(parent);
                        if (!parent.empty())
                            info.parents.push_back(parent);
                    }
                }
                else if (key == "Directories" || key == "ScaledDirectories")
                {
                    std::istringstream ss(value);
                    std::string dir;
                    while (std::getline(ss, dir, ','))
                    {
                        dir = trim(dir);
                        if (!dir.empty())
                            directory_list.push_back(dir);
                    }
                }
            }
            else
            {
                sections[current_section][key] = value;
            }
        }

        // Build IconDirectory entries
        for (const auto &dir_name : directory_list)
        {
            auto sec_it = sections.find(dir_name);
            if (sec_it == sections.end())
                continue;

            const auto &sec = sec_it->second;

            IconDirectory idir;
            idir.path = dir_name;

            auto size_it = sec.find("Size");
            if (size_it != sec.end())
            {
                try
                {
                    idir.size = std::stoi(size_it->second);
                }
                catch (...)
                {
                    continue;
                }
            }
            else
                continue; // Size is required

            auto type_it = sec.find("Type");
            if (type_it != sec.end())
            {
                if (type_it->second == "Fixed")
                    idir.type = DirectoryType::Fixed;
                else if (type_it->second == "Scalable")
                    idir.type = DirectoryType::Scalable;
                else
                    idir.type = DirectoryType::Threshold;
            }

            auto min_it = sec.find("MinSize");
            try
            {
                idir.min_size = (min_it != sec.end()) ? std::stoi(min_it->second) : idir.size;
            }
            catch (...)
            {
                idir.min_size = idir.size;
            }

            auto max_it = sec.find("MaxSize");
            try
            {
                idir.max_size = (max_it != sec.end()) ? std::stoi(max_it->second) : idir.size;
            }
            catch (...)
            {
                idir.max_size = idir.size;
            }

            auto thresh_it = sec.find("Threshold");
            try
            {
                idir.threshold = (thresh_it != sec.end()) ? std::stoi(thresh_it->second) : 2;
            }
            catch (...)
            {
                idir.threshold = 2;
            }

            info.directories.push_back(idir);
        }

        s_theme_cache[theme_dir] = info;
        return info;
    }

    // ---------------------------------------------------------------------------
    // directory_matches_size
    // ---------------------------------------------------------------------------
    bool IconThemeLookup::directory_matches_size(const IconDirectory &dir, int size)
    {
        switch (dir.type)
        {
        case DirectoryType::Fixed:
            return dir.size == size;
        case DirectoryType::Scalable:
            return size >= dir.min_size && size <= dir.max_size;
        case DirectoryType::Threshold:
            return size >= (dir.size - dir.threshold) && size <= (dir.size + dir.threshold);
        }
        return false;
    }

    // ---------------------------------------------------------------------------
    // directory_size_distance
    // ---------------------------------------------------------------------------
    int IconThemeLookup::directory_size_distance(const IconDirectory &dir, int size)
    {
        switch (dir.type)
        {
        case DirectoryType::Fixed:
            return std::abs(dir.size - size);
        case DirectoryType::Scalable:
            if (size < dir.min_size)
                return dir.min_size - size;
            if (size > dir.max_size)
                return size - dir.max_size;
            return 0;
        case DirectoryType::Threshold:
            if (size < (dir.size - dir.threshold))
                return (dir.size - dir.threshold) - size;
            if (size > (dir.size + dir.threshold))
                return size - (dir.size + dir.threshold);
            return 0;
        }
        return INT_MAX;
    }

    // ---------------------------------------------------------------------------
    // try_file_extensions — check for .png and .svg
    // ---------------------------------------------------------------------------
    std::string IconThemeLookup::try_file_extensions(const std::string &base_path)
    {
        // Prefer SVG for scalability, then PNG
        std::string svg_path = base_path + ".svg";
        if (fs::exists(svg_path))
            return svg_path;

        std::string png_path = base_path + ".png";
        if (fs::exists(png_path))
            return png_path;

        return "";
    }

    // ---------------------------------------------------------------------------
    // lookup_icon_in_theme
    // ---------------------------------------------------------------------------
    std::string IconThemeLookup::lookup_icon_in_theme(const std::string &icon_name, int size,
                                                      const std::string &theme_name,
                                                      const std::vector<std::string> &base_dirs)
    {
        // Find the theme directory
        std::string theme_dir;
        for (const auto &base : base_dirs)
        {
            std::string candidate = base + "/" + theme_name;
            if (fs::is_directory(candidate))
            {
                theme_dir = candidate;
                break;
            }
        }

        if (theme_dir.empty())
            return "";

        ThemeInfo info = parse_index_theme(theme_dir);

        // Step 1: Find the directory with the minimum distance
        int min_distance = INT_MAX;
        std::string best_path;

        for (const auto &dir : info.directories)
        {
            std::string base_path = theme_dir + "/" + dir.path + "/" + icon_name;
            std::string result = try_file_extensions(base_path);
            if (!result.empty())
            {
                int dist = directory_size_distance(dir, size);
                if (dist < min_distance)
                {
                    min_distance = dist;
                    best_path = result;
                }

                // If we found a perfect match (distance 0), we can stop
                if (min_distance == 0)
                    return best_path;
            }
        }

        if (!best_path.empty())
            return best_path;

        // Step 3: Try parent themes (Inherits)
        for (const auto &parent : info.parents)
        {
            if (parent == theme_name)
                continue; // avoid infinite loop

            std::string result = lookup_icon_in_theme(icon_name, size, parent, base_dirs);
            if (!result.empty())
                return result;
        }

        return "";
    }

    // ---------------------------------------------------------------------------
    // lookup_fallback_icon — search in /usr/share/pixmaps
    // ---------------------------------------------------------------------------
    std::string IconThemeLookup::lookup_fallback_icon(const std::string &icon_name,
                                                      const std::vector<std::string> &base_dirs)
    {
        // Try pixmaps directories
        std::vector<std::string> pixmap_dirs = {"/usr/share/pixmaps", "/usr/local/share/pixmaps"};

        for (const auto &dir : pixmap_dirs)
        {
            if (!fs::is_directory(dir))
                continue;

            std::string result = try_file_extensions(dir + "/" + icon_name);
            if (!result.empty())
                return result;
        }

        return "";
    }

    // ---------------------------------------------------------------------------
    // find_icon — main entry point
    // ---------------------------------------------------------------------------
    std::string IconThemeLookup::find_icon(const std::string &icon_name, int size,
                                           const std::string &theme)
    {
        if (icon_name.empty())
            return "";

        // If it's an absolute path, return it if it exists
        if (icon_name[0] == '/' && fs::exists(icon_name))
        {
            return icon_name;
        }

        std::string cache_key = icon_name + ":" + std::to_string(size) + ":" + theme;
        if (s_resolution_cache.count(cache_key))
            return s_resolution_cache[cache_key];

        // LOG_INFO << "[IconThemeLookup] Finding icon for: \"" << icon_name << "\" (size: " << size
        //          << ")";

        // 1. Search requested theme with the ORIGINAL name FIRST
        std::string theme_name = theme.empty() ? get_active_theme_name() : theme;
        auto base_dirs = get_base_dirs();
        std::string result = lookup_icon_in_theme(icon_name, size, theme_name, base_dirs);
        if (!result.empty())
        {
            s_resolution_cache[cache_key] = result;
            return result;
        }

        // 2. Resolve through .desktop files as FALLBACK
        // This ensures that if app_id is "firefox", we look into firefox.desktop and find
        // Icon=browser-firefox
        std::string desktop_icon = DesktopEntry::get_icon_name(icon_name);
        if (!desktop_icon.empty() && desktop_icon != icon_name)
        {
            LOG_INFO << "[IconThemeLookup] Fallback for " << icon_name
                     << ": found actual icon name \"" << desktop_icon << "\" in desktop file.";
            result = lookup_icon_in_theme(desktop_icon, size, theme_name, base_dirs);
            if (!result.empty())
            {
                s_resolution_cache[cache_key] = result;
                return result;
            }

            // If found in desktop but NOT in theme, try fallback directories for the new name
            result = lookup_fallback_icon(desktop_icon, base_dirs);
            if (!result.empty())
            {
                s_resolution_cache[cache_key] = result;
                return result;
            }
        }

        // 3. Fallback to hicolor
        if (theme_name != "hicolor")
        {
            result = lookup_icon_in_theme(icon_name, size, "hicolor", base_dirs);
            if (!result.empty())
                return result;

            if (!desktop_icon.empty() && desktop_icon != icon_name)
            {
                result = lookup_icon_in_theme(desktop_icon, size, "hicolor", base_dirs);
                if (!result.empty())
                {
                    s_resolution_cache[cache_key] = result;
                    return result;
                }
            }
        }

        // 4. Fallback to pixmaps
        result = lookup_fallback_icon(icon_name, base_dirs);
        if (!result.empty())
        {
            s_resolution_cache[cache_key] = result;
            return result;
        }

        if (!desktop_icon.empty() && desktop_icon != icon_name)
        {
            result = lookup_fallback_icon(desktop_icon, base_dirs);
            if (!result.empty())
            {
                s_resolution_cache[cache_key] = result;
                return result;
            }
        }

        s_resolution_cache[cache_key] = "";
        return "";
    }

} // namespace horizon
