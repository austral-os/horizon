#pragma once

#include <string>
#include <map>

namespace horizon
{
    /**
     * @class XdgUserDirs
     * @brief Utility specialized in retrieving paths for XDG User Directories.
     * 
     * It parses ~/.config/user-dirs.dirs to find the localized paths for 
     * Desktop, Download, Documents, etc.
     */
    class XdgUserDirs
    {
    public:
        static std::string get_desktop();
        static std::string get_download();
        static std::string get_templates();
        static std::string get_public_share();
        static std::string get_documents();
        static std::string get_music();
        static std::string get_pictures();
        static std::string get_videos();

        /**
         * @brief Generic method to get any XDG directory by its key.
         * @param key The key (e.g., "DESKTOP", "DOWNLOAD")
         * @return The absolute path to the directory.
         */
        static std::string get_path(const std::string &key);

    private:
        static void ensure_loaded();
        static std::map<std::string, std::string> m_dirs;
        static bool m_loaded;
    };
} // namespace horizon
