cmake_minimum_required(VERSION 3.10)
project(terminal)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
    ${PROJECT_SOURCE_DIR}/../../libs/horizon-terminal/include
)

add_executable(terminal ${SOURCES})

target_link_libraries(terminal
    PRIVATE
        horizon
        horizon-terminal
)

horizon_install_app(terminal
    APP_ID "terminal"
    NAME "Terminal"
    COMMENT "Horizon Terminal Emulator"
    ICON "horizon-terminal"
    VERSION "0.1.6"
    CATEGORIES "System;TerminalEmulator;"
    MIMETYPE "x-scheme-handler/ssh;"
)

install(DIRECTORY assets/color-schemes/
    DESTINATION share/horizon/terminal/color-schemes
    COMPONENT terminal
)
