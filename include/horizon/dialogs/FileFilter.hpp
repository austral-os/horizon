#pragma once

#include <string>
#include <vector>

namespace horizon
{
    enum class FileFilterUsage
    {
        All,
        Open,
        Save
    };

    struct FileFilter
    {
        std::string name;
        std::vector<std::string> patterns;
        FileFilterUsage usage{FileFilterUsage::All};
    };
} // namespace horizon
