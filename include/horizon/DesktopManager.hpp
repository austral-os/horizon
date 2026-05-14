#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace horizon
{
    /**
     * @struct DesktopEntry
     * @brief Represents the data from a .desktop file.
     */
    struct DesktopEntry
    {
        std::string id;      // desktop filename (e.g., firefox.desktop)
        std::string name;    // Name field
        std::string icon;    // Icon field
        std::string exec;    // Exec field
        std::vector<std::string> mime_types; // MimeType field
        std::string path;    // Full local path
    };

    /**
     * @class DesktopManager
     * @brief Utility class for managing and parsing .desktop files and MIME associations.
     */
    class DesktopManager
    {
    public:
        /**
         * @brief Returns a prioritized list of applications that can open the given MIME type.
         */
        static std::vector<DesktopEntry> get_apps_for_mime(const std::string& mime_type);
        
        /**
         * @brief Returns the MIME type of a file.
         */
        static std::string get_mime_type(const std::string& path);
        
        /**
         * @brief Loads all available desktop entries from standard system paths.
         */
        static std::vector<DesktopEntry> load_all_desktop_entries();
        
        static void add_mime_association(const std::string& mime_type, const std::string& desktop_id);
        static void remove_mime_association(const std::string& mime_type, const std::string& desktop_id);
        static void set_default_application(const std::string& mime_type, const std::string& desktop_id);

        static std::vector<DesktopEntry> load_autostart_entries();
        static void add_to_autostart(const DesktopEntry& entry);
        static void update_autostart_cmd(const std::string& path, const std::string& new_cmd);
        static void remove_from_autostart(const std::string& path);

        // --- Utility methods moved from the old DesktopEntry class ---
        
        static std::string get_icon_name(const std::string &app_id);
        static std::string find_desktop_file(const std::string &app_id);
        static std::string get_exec_command(const std::string &app_id);
        static std::string get_exec_command_from_path(const std::string &path);
        static std::string get_value_from_desktop_file(const std::string &path, const std::string &key);
        
        static void add_search_path(const std::string &path);
        static void set_search_paths(const std::vector<std::string> &paths);
        static std::vector<std::string> get_desktop_search_dirs();

    private:
        static std::optional<DesktopEntry> parse_desktop_file(const std::string& path);
        
        struct MimeAssociations
        {
            std::vector<std::string> default_apps;
            std::vector<std::string> added_apps;
            std::vector<std::string> removed_apps;
        };
        
        static MimeAssociations load_mime_associations(const std::string& mime_type);
        static void parse_mimeapps_list(const std::string& path, const std::string& mime_type, MimeAssociations& out);

        static std::vector<std::string> s_additional_search_paths;
        static std::map<std::string, std::string> s_desktop_file_cache;
        static std::map<std::string, std::string> s_icon_name_cache;
    };
} // namespace horizon
