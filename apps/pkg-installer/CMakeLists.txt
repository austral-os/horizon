cmake_minimum_required(VERSION 3.10)
project(pkg-installer)

find_package(PkgConfig REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(pkg-installer ${SOURCES})

target_include_directories(pkg-installer
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
)

target_link_libraries(pkg-installer
    PRIVATE
        horizon
)

horizon_install_app(pkg-installer
    APP_ID "pkg-installer"
    NAME "Package Installer"
    COMMENT "Install .deb packages easily"
    ICON "pkg-installer"
    VERSION "0.1.0"
    CATEGORIES "System;Utility;"
    MIMETYPE "application/vnd.debian.binary-package;application/x-debian-package;"
    EXEC_ARGS "%f"
)
