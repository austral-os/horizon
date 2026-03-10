#include "DesktopParser.hpp"
#include <fstream>
#include <horizon/Logger.hpp>
#include <iostream>

std::optional<DesktopEntry> DesktopParser::parse(const std::string &app_name)
{
    std::string path =
        "/home/horacio/Desarrollo/austral-os/horizon/examples/config/apps/" + app_name + ".desktop";
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR << "[DesktopParser] Could not find desktop file for: " << app_name << std::endl;
        return std::nullopt;
    }

    DesktopEntry entry;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.substr(0, 5) == "Exec=")
        {
            entry.exec = line.substr(5);
        }
        else if (line.substr(0, 5) == "Name=")
        {
            entry.name = line.substr(5);
        }
        else if (line.substr(0, 5) == "Icon=")
        {
            entry.icon = line.substr(5);
        }
    }

    if (entry.exec.empty())
    {
        LOG_ERROR << "[DesktopParser] Could not find Exec in desktop file for: " << app_name
                  << std::endl;
        return std::nullopt;
    }

    return entry;
}
