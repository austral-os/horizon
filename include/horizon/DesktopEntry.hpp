#pragma once

#include <string>
#include <vector>

namespace horizon
{
    /**
     * @class DesktopEntry
     * @brief Utility class for parsing .desktop files to extract metadata like icons.
     */
    class DesktopEntry
    {
    public:
        /**
         * @brief Extracts the icon name from a desktop file associated with an app_id.
         * @param app_id The application identifier (e.g., "konqbrowser").
         * @return The icon name if found, or an empty string.
         */
        static std::string get_icon_name(const std::string &app_id);

        /**
         * @brief Finds the absolute path to a .desktop file for a given app_id.
         * @param app_id The application identifier.
         * @return The absolute path to the .desktop file, or empty string if not found.
         */
        static std::string find_desktop_file(const std::string &app_id);

    private:
        static std::vector<std::string> get_desktop_search_dirs();
    };
} // namespace horizon
