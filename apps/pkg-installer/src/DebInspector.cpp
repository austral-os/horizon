#include "DebInspector.hpp"
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <horizon/IconThemeLookup.hpp>

namespace horizon {

static std::string exec(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::optional<DebInfo> DebInspector::inspect(const std::string& deb_path) {
    if (!std::filesystem::exists(deb_path)) return std::nullopt;

    DebInfo info;
    info.package_name = extract_control_field(deb_path, "Package");
    info.version = extract_control_field(deb_path, "Version");
    info.description = extract_control_field(deb_path, "Description");

    if (info.package_name.empty()) return std::nullopt;

    // Try to find icon in control file first
    std::string icon_field = extract_control_field(deb_path, "Icon");
    if (!icon_field.empty()) {
        info.icon_path = icon_field;
        info.icon_is_theme_name = true; // Usually control icons are names
    } else {
        // Look in theme by package name
        std::string theme_icon = IconThemeLookup::find_icon(info.package_name, 64);
        if (!theme_icon.empty()) {
            info.icon_path = theme_icon;
            info.icon_is_theme_name = false; // It's a path now
        } else {
            // Fallback to searching inside the package
            info.icon_path = find_icon_in_package(deb_path, info.package_name);
            info.icon_is_theme_name = false;
        }
    }

    return info;
}

std::string DebInspector::extract_control_field(const std::string& deb_path, const std::string& field) {
    std::string cmd = "dpkg-deb -f \"" + deb_path + "\" " + field + " 2>/dev/null";
    std::string out = exec(cmd);
    // Remove newline
    if (!out.empty() && out.back() == '\n') out.pop_back();
    return out;
}

std::string DebInspector::find_icon_in_package(const std::string& deb_path, const std::string& package_name) {
    // This is a bit expensive, we list files and look for icons
    std::string cmd = "dpkg-deb -c \"" + deb_path + "\" | grep -E '\\.(png|svg)$' | grep -i 'icon' | head -n 1";
    std::string line = exec(cmd);
    if (line.empty()) return "";

    // Parse the path from dpkg-deb -c output (last column)
    std::stringstream ss(line);
    std::string part;
    std::string last_part;
    while (ss >> part) last_part = part;

    if (last_part.empty()) return "";

    // Extract this specific file to a temp location
    std::string temp_dir = "/tmp/horizon-pkg-installer/" + package_name;
    std::filesystem::create_directories(temp_dir);
    std::string out_path = temp_dir + "/icon" + std::filesystem::path(last_part).extension().string();
    
    // We need to use tar to extract just one file from the data member
    std::string extract_cmd = "dpkg-deb --fsys-tarfile \"" + deb_path + "\" | tar -xO " + last_part + " > \"" + out_path + "\"";
    std::system(extract_cmd.c_str());

    return out_path;
}

} // namespace horizon
