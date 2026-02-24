cmake_minimum_required(VERSION 3.10)
project(Horizon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor)
pkg_check_modules(CAIRO REQUIRED cairo)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

file(GLOB_RECURSE SOURCES src/*.cpp src/xdg-shell-client-protocol.c)

add_library(horizon STATIC ${SOURCES})

# Headers públicos de tu librería
target_include_directories(horizon
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${WAYLAND_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
)

target_link_libraries(horizon
    PRIVATE
        ${WAYLAND_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${nlohmann_json_LIBRARIES}
)

add_executable(minimal
    examples/minimal/main.cpp
)

target_link_libraries(minimal
    PRIVATE
        horizon
)

# Agregamos un target "run" para compilar y ejecutar de una vez
add_custom_target(run
    COMMAND $<TARGET_FILE:minimal>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS minimal
    COMMENT "Ejecutando la demo minimal..."
)