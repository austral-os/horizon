#pragma once

#include <string>
#include <vector>
#include <optional>

namespace horizon::preferences
{
    struct DesktopEntry
    {
        std::string id;      // desktop filename (e.g., firefox.desktop)
        std::string name;    // Name field
        std::string icon;    // Icon field
        std::string exec;    // Exec field
        std::vector<std::string> mime_types; // MimeType field
        std::string path;    // Full local path
    };

    class DesktopManager
    {
    public:
        /**
         * @brief Returns a prioritized list of applications that can open the given MIME type.
         */
        static std::vector<DesktopEntry> get_apps_for_mime(const std::string& mime_type);
        static std::vector<DesktopEntry> load_all_desktop_entries();
        static void add_mime_association(const std::string& mime_type, const std::string& desktop_id);
        static void remove_mime_association(const std::string& mime_type, const std::string& desktop_id);
        static void set_default_application(const std::string& mime_type, const std::string& desktop_id);

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
    };
} // namespace horizon::preferences
