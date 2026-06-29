cmake_minimum_required(VERSION 3.10)
project(text-editor)

find_package(PkgConfig REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(text-editor ${SOURCES})

target_include_directories(text-editor
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
)

target_link_libraries(text-editor
    PRIVATE
        horizon
        horizon-text
)

horizon_install_app(text-editor
    APP_ID "text-editor"
    NAME "Text Editor"
    COMMENT "Standard text editor"
    ICON "text-editor"
    VERSION "0.1.5"
    CATEGORIES "Utility;TextEditor;"
    MIMETYPE "text/plain;text/markdown;application/x-zerosize;"
    EXEC_ARGS "%F"
)
