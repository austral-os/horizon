cmake_minimum_required(VERSION 3.10)
project(horizon-keyring-manager)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
)

add_executable(horizon-keyring-manager ${SOURCES})

target_link_libraries(horizon-keyring-manager
    PRIVATE
        horizon
)

horizon_install_app(horizon-keyring-manager
    APP_ID "keyring-manager"
    NAME "Passwords and Keys"
    COMMENT "Manage your passwords and encryption keys"
    ICON "dialog-password"
    VERSION "0.1.0"
    CATEGORIES "Settings;Security;"
)
