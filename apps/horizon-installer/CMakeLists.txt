cmake_minimum_required(VERSION 3.10)
project(horizon-installer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(horizon-installer
    main.cpp
    Wizard.cpp
    pages/LanguagePage/LanguagePage.cpp
    pages/WelcomePage/WelcomePage.cpp
    pages/LicensePage/LicensePage.cpp
    pages/DiskPage/DiskPage.cpp
    pages/InstallPage/InstallPage.cpp
    pages/RegionPage/RegionPage.cpp
    pages/UserPage/UserPage.cpp
    pages/SuccessPage/SuccessPage.cpp
)

target_link_libraries(horizon-installer
    PRIVATE
        horizon
        horizon-disk-utilities
        horizon-installer-utils
        nlohmann_json::nlohmann_json
)

horizon_install_app(horizon-installer
    APP_ID "horizon-installer"
    NAME "OS Installer"
    COMMENT "Install Austral OS to your computer"
    ICON "emblem-austral"
    VERSION "0.1.0"
    CATEGORIES "System;Settings;Utility;"
)
