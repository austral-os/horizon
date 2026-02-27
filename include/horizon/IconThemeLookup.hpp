#pragma once

#include <string>
#include <vector>

namespace horizon
{

    /**
     * @brief Resolves icon names to file paths following the freedesktop.org Icon Theme
     * Specification.
     *
     * Supports PNG and SVG icon formats. Searches the active GTK icon theme,
     * falls back to "hicolor", and then to /usr/share/pixmaps.
     */
    class IconThemeLookup
    {
    public:
        /**
         * @brief Find an icon file path by name and desired size.
         *
         * @param icon_name The icon name (e.g. "folder", "firefox", "terminal").
         * @param size      The desired icon size in pixels (e.g. 16, 24, 32, 48).
         * @param theme     Optional theme name override. If empty, uses the active GTK theme.
         * @return The absolute path to the icon file, or empty string if not found.
         */
        static std::string find_icon(const std::string &icon_name, int size,
                                     const std::string &theme = "");

    private:
        enum class DirectoryType
        {
            Fixed,
            Scalable,
            Threshold
        };

        struct IconDirectory
        {
            std::string path; // subdirectory relative to theme root (e.g. "48x48/apps")
            int size{0};      // nominal size
            int min_size{0};  // for Scalable
            int max_size{0};  // for Scalable
            int threshold{2}; // for Threshold
            DirectoryType type{DirectoryType::Threshold};
        };

        struct ThemeInfo
        {
            std::string name;
            std::vector<std::string> parents; // Inherits= list
            std::vector<IconDirectory> directories;
        };

        static std::string get_active_theme_name();
        static std::vector<std::string> get_base_dirs();

        static ThemeInfo parse_index_theme(const std::string &theme_dir);

        static std::string lookup_icon_in_theme(const std::string &icon_name, int size,
                                                const std::string &theme_name,
                                                const std::vector<std::string> &base_dirs);

        static std::string lookup_fallback_icon(const std::string &icon_name,
                                                const std::vector<std::string> &base_dirs);

        static bool directory_matches_size(const IconDirectory &dir, int size);
        static int directory_size_distance(const IconDirectory &dir, int size);

        static std::string try_file_extensions(const std::string &base_path);
    };

} // namespace horizon
