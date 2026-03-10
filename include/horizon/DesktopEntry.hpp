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

        /**
         * @brief Gets the execution command for a given app_id.
         * @param app_id The application identifier.
         * @return The execution command, or empty string if not found.
         */
        static std::string get_exec_command(const std::string &app_id);

        /**
         * @brief Gets the execution command from a desktop file at the given path.
         * @param path The absolute path to the .desktop file.
         * @return The execution command, or empty string if not found.
         */
        static std::string get_exec_command_from_path(const std::string &path);

        /**
         * @brief Adds a directory to the list of additional search paths for .desktop files.
         * @param path The absolute path to the directory.
         */
        static void add_search_path(const std::string &path);

        /**
         * @brief Sets the list of additional search paths for .desktop files.
         * @param paths A vector of absolute paths to directories.
         */
        static void set_search_paths(const std::vector<std::string> &paths);

        static std::vector<std::string> get_desktop_search_dirs();
        static std::string get_value_from_desktop_file(const std::string &path,
                                                       const std::string &key);

    private:
        static std::vector<std::string> s_additional_search_paths;
    };
} // namespace horizon
