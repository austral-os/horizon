#pragma once

#include "horizon/arkutils/FileSystemModel.hpp"
#include <string>

namespace horizon::arkfm
{
    class ArkfmFileProvider
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
    };
} // namespace horizon::arkfm
