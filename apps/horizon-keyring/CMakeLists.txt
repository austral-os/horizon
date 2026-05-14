cmake_minimum_required(VERSION 3.10)
project(horizon-keyring)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
)

add_executable(horizon-keyring ${SOURCES})

target_link_libraries(horizon-keyring
    PRIVATE
        horizon
        sqlite3
        crypto
        ssl
)

horizon_install_app(horizon-keyring
    APP_ID "keyring"
    NAME "Horizon Keyring"
    COMMENT "Horizon Secret Service Daemon"
    ICON "dialog-password"
    VERSION "0.1.0"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)

# Install D-Bus service file
install(FILES org.freedesktop.secrets.service
    DESTINATION share/dbus-1/services
    COMPONENT keyring
)

# Install Systemd user service file
install(FILES horizon-keyring.service
    DESTINATION lib/systemd/user
    COMPONENT keyring
)

add_subdirectory(pam)
