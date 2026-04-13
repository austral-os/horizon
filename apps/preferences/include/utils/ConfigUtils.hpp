#pragma once

#include <string>
#include <cstdlib>
#include <filesystem>

namespace horizon::preferences
{
    /**
     * @brief Helper to get the absolute path for a configuration file in ~/.config/horizon/
     * @param filename The name of the file (e.g. "dock.json")
     * @return std::string The absolute path to the configuration file.
     */
    inline std::string get_config_path(const std::string& filename)
    {
        const char* home = std::getenv("HOME");
        if (!home) return filename;

        std::filesystem::path p(home);
        p /= ".config/horizon";
        p /= filename;
        return p.string();
    }
} // namespace horizon::preferences
