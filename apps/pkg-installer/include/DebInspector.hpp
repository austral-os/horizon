#pragma once

#include <string>
#include <vector>
#include <optional>

namespace horizon {

struct DebInfo {
    std::string package_name;
    std::string version;
    std::string description;
    std::string icon_path; // Path to extracted icon or theme icon name
    bool icon_is_theme_name = false;
};

class DebInspector {
public:
    static std::optional<DebInfo> inspect(const std::string& deb_path);

private:
    static std::string extract_control_field(const std::string& deb_path, const std::string& field);
    static std::string find_icon_in_package(const std::string& deb_path, const std::string& package_name);
};

} // namespace horizon
