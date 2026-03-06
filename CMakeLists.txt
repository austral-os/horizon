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
    "src/core/*.cpp"
    "src/core/IconView.cpp"
    "src/xdg-activation-v1-protocol.c"
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

add_executable(top_panel
    examples/top_panel/main.cpp
)

target_link_libraries(top_panel
    PRIVATE
        horizon
)

add_executable(menubar_demo
    examples/menubar_demo.cpp
)

target_link_libraries(menubar_demo
    PRIVATE
        horizon
)

add_executable(horizon_menu_manager_d
    examples/horizon_menu_manager_d/main.cpp
)

target_link_libraries(horizon_menu_manager_d
    PRIVATE
        horizon
)

add_executable(dock
    examples/dock/main.cpp
)

target_link_libraries(dock
    PRIVATE
        horizon
)

add_executable(horizon_wall
    examples/horizon_wall/main.cpp
)

target_link_libraries(horizon_wall
    PRIVATE
        horizon
)

add_executable(app_manager
    examples/app_manager/main.cpp
    examples/app_manager/AppManager.cpp
)

target_link_libraries(app_manager
    PRIVATE
        horizon
)

add_executable(image_demo
    examples/image_demo.cpp
)

target_link_libraries(image_demo
    PRIVATE
        horizon
)

add_executable(iconview_demo
    examples/iconview_demo.cpp
)

target_link_libraries(iconview_demo
    PRIVATE
        horizon
)

add_executable(horizon_session
    examples/horizon_session/main.cpp
    examples/horizon_session/HorizonSession.cpp
    examples/horizon_session/DesktopParser.cpp
    examples/horizon_session/Logger.cpp
)

target_link_libraries(horizon_session
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