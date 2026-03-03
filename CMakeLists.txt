cmake_minimum_required(VERSION 3.10)
project(Horizon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(RSVG REQUIRED librsvg-2.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    "src/*.cpp" 
    "src/*.c"
    "include/horizon/*.hpp"
)

add_library(horizon STATIC ${SOURCES})

# Headers públicos de tu librería
target_include_directories(horizon
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${WAYLAND_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
        ${RSVG_INCLUDE_DIRS}
)

target_link_libraries(horizon
    PRIVATE
        ${WAYLAND_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
        ${nlohmann_json_LIBRARIES}
        ${RSVG_LIBRARIES}
)

add_executable(minimal
    examples/minimal/main.cpp
)

target_link_libraries(minimal
    PRIVATE
        horizon
)

add_executable(polygon_demo
    examples/polygon_demo.cpp
)

target_link_libraries(polygon_demo
    PRIVATE
        horizon
)

add_executable(main_window
    examples/main_window/main_window.cpp
)

target_link_libraries(main_window
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