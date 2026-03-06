#pragma once
#include <optional>
#include <string>

struct DesktopEntry
{
    std::string name;
    std::string exec;
    std::string icon;
};

class DesktopParser
{
public:
    static std::optional<DesktopEntry> parse(const std::string &app_name);
};
