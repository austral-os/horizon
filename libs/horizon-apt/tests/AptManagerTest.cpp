#include <horizon/apt/AptManager.hpp>
#include <iostream>

using namespace horizon::apt;

int main() {
    AptManager apt;
    if (!apt.initialize()) {
        std::cerr << "Failed to initialize AptManager" << std::endl;
        return 1;
    }

    std::cout << "Successfully initialized AptManager." << std::endl;

    auto info = apt.get_package_info("bash");
    if (info) {
        std::cout << "Found bash: " << info->version << std::endl;
        std::cout << "Installed: " << (info->is_installed ? "Yes" : "No") << std::endl;
    } else {
        std::cerr << "Failed to find 'bash' package." << std::endl;
        return 1;
    }

    auto results = apt.search_packages("nano");
    std::cout << "Found " << results.size() << " packages matching 'nano'" << std::endl;
    
    return 0;
}
