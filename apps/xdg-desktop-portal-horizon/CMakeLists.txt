cmake_minimum_required(VERSION 3.10)
project(xdg-desktop-portal-horizon)

find_package(PkgConfig REQUIRED)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
)

add_executable(xdg-desktop-portal-horizon ${SOURCES})

target_link_libraries(xdg-desktop-portal-horizon
    PRIVATE
        horizon
)

horizon_install_app(xdg-desktop-portal-horizon
    APP_ID "portal"
    NAME "Horizon Desktop Portal"
    COMMENT "Horizon XDG Desktop Portal Backend"
    VERSION "0.1.0"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)

# Install D-Bus service file
install(FILES org.freedesktop.impl.portal.desktop.horizon.service
    DESTINATION share/dbus-1/services
    COMPONENT portal
)

# Install portal configuration file
install(FILES horizon.portal
    DESTINATION share/xdg-desktop-portal/portals
    COMPONENT portal
)

# Install desktop-specific portals.conf
install(FILES horizon-portals.conf
    DESTINATION share/xdg-desktop-portal
    COMPONENT portal
)
