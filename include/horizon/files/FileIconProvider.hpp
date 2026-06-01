#pragma once

#include "horizon/arkutils/FileInfo.hpp"
#include <string>

namespace horizon::files
{
    class FileIconProvider
    {
    public:
        /**
         * Returns the icon name for a given file info.
         * @param f The file info object.
         * @return The name of the icon to be used.
         */
        static std::string get_icon_name(const arkutils::FileInfo &f);

        /**
         * Returns the display name for a given file info.
         * @param f The file info object.
         * @return The display name (extracted from .desktop if applicable).
         */
        static std::string get_display_name(const arkutils::FileInfo &f);

        /**
         * Sets whether file extensions should be shown globally in the UI.
         */
        static void set_show_extensions(bool show);
    };
} // namespace horizon::files
