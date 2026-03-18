#include <horizon/XdgUserDirs.hpp>
#include <fstream>
#include <cstdlib>
#include <filesystem>

namespace horizon
{
    std::map<std::string, std::string> XdgUserDirs::m_dirs;
    bool XdgUserDirs::m_loaded = false;

    void XdgUserDirs::ensure_loaded()
    {
        if (m_loaded)
            return;

        const char *home = getenv("HOME");
        if (!home)
            return;

        std::string config_path = std::string(home) + "/.config/user-dirs.dirs";
        std::ifstream file(config_path);
        if (!file.is_open())
        {
            m_loaded = true;
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            // Lines are in the format XDG_XXX_DIR="$HOME/YYY"
            if (line.find("XDG_") == 0)
            {
                size_t eq_pos = line.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key = line.substr(4, eq_pos - 8); // Extract "XXX" from "XDG_XXX_DIR"
                    std::string val = line.substr(eq_pos + 1);

                    // Remove quotes
                    if (val.size() >= 2 && val[0] == '"' && val.back() == '"')
                    {
                        val = val.substr(1, val.size() - 2);
                    }

                    // Replace $HOME
                    size_t home_pos = val.find("$HOME");
                    if (home_pos != std::string::npos)
                    {
                        val.replace(home_pos, 5, home);
                    }

                    m_dirs[key] = val;
                }
            }
        }

        m_loaded = true;
    }

    std::string XdgUserDirs::get_path(const std::string &key)
    {
        ensure_loaded();
        auto it = m_dirs.find(key);
        if (it != m_dirs.end())
        {
            return it->second;
        }

        // Fallback to standard names if not found or failed to load
        const char *home = getenv("HOME");
        if (!home) return "";

        std::filesystem::path home_path(home);
        if (key == "DESKTOP") return (home_path / "Desktop").string();
        if (key == "DOWNLOAD") return (home_path / "Downloads").string();
        if (key == "DOCUMENTS") return (home_path / "Documents").string();
        if (key == "MUSIC") return (home_path / "Music").string();
        if (key == "PICTURES") return (home_path / "Pictures").string();
        if (key == "VIDEOS") return (home_path / "Videos").string();
        if (key == "TEMPLATES") return (home_path / "Templates").string();
        if (key == "PUBLICSHARE") return (home_path / "Public").string();

        return "";
    }

    std::string XdgUserDirs::get_desktop() { return get_path("DESKTOP"); }
    std::string XdgUserDirs::get_download() { return get_path("DOWNLOAD"); }
    std::string XdgUserDirs::get_templates() { return get_path("TEMPLATES"); }
    std::string XdgUserDirs::get_public_share() { return get_path("PUBLICSHARE"); }
    std::string XdgUserDirs::get_documents() { return get_path("DOCUMENTS"); }
    std::string XdgUserDirs::get_music() { return get_path("MUSIC"); }
    std::string XdgUserDirs::get_pictures() { return get_path("PICTURES"); }
    std::string XdgUserDirs::get_videos() { return get_path("VIDEOS"); }

} // namespace horizon
