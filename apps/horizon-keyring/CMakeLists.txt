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

add_subdirectory(pam)
