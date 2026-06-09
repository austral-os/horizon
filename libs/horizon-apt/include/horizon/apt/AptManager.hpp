#pragma once

#include <string>
#include <vector>
#include <optional>

namespace horizon::apt {

struct PackageInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string icon;
    bool is_installed;
};

class AptManager {
public:
    AptManager();
    ~AptManager();

    // Initializes the APT cache. Returns true on success.
    bool initialize();

    // Search for packages by keyword.
    std::vector<PackageInfo> search_packages(const std::string& keyword);

    // List packages filtered by sections. If sections is empty, returns general packages.
    std::vector<PackageInfo> list_packages_by_sections(const std::vector<std::string>& sections, int max_results = 50);

    // Get detailed info for a specific package.
    std::optional<PackageInfo> get_package_info(const std::string& pkg_name);

    // Install a package using pkexec. Returns true on success.
    bool install_package(const std::string& pkg_name);

    // Remove a package using pkexec. Returns true on success.
    bool remove_package(const std::string& pkg_name);

private:
    struct Private;
    Private* d;
};

} // namespace horizon::apt
