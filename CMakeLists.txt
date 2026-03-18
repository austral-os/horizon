cmake_minimum_required(VERSION 3.10)
project(Horizon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor wayland-egl)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(RSVG REQUIRED librsvg-2.0)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLESV2 REQUIRED glesv2)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c"
    "src/core/*.cpp"
    "src/protocols/*.c"
)

add_library(horizon STATIC ${SOURCES})

# Headers públicos de tu librería
target_include_directories(horizon
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
    PRIVATE
        ${WAYLAND_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
        ${RSVG_INCLUDE_DIRS}
        ${EGL_INCLUDE_DIRS}
        ${GLESV2_INCLUDE_DIRS}
)

target_link_libraries(horizon
    PRIVATE
        ${WAYLAND_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
        ${nlohmann_json_LIBRARIES}
        ${RSVG_LIBRARIES}
        ${EGL_LIBRARIES}
        ${GLESV2_LIBRARIES}
)

add_executable(minimal
    examples/minimal/main.cpp
)

target_link_libraries(minimal
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

add_subdirectory(examples/top_panel)

add_subdirectory(examples/dock)

add_subdirectory(examples/horizon_wall)


add_executable(horizon_session
    examples/horizon_session/main.cpp
    examples/horizon_session/HorizonSession.cpp
    examples/horizon_session/DesktopParser.cpp
    examples/horizon_session/IpcServer.cpp
)

target_link_libraries(horizon_session
    PRIVATE
        horizon
)


add_executable(aboutus
    examples/aboutus/main.cpp
    examples/aboutus/AboutWindow.cpp
)

target_link_libraries(aboutus
    PRIVATE
        horizon
)

add_executable(files_demo
    examples/files-demo/FilesDemo.cpp
)

target_link_libraries(files_demo
    PRIVATE
        horizon
)

add_subdirectory(examples/arkfm)

add_executable(mouse_wheel_test
    tests/mouse_wheel_test.cpp
)

target_link_libraries(mouse_wheel_test
    PRIVATE
        horizon
)

add_executable(absolute_position_test
    tests/absolute_position_test.cpp
)

target_link_libraries(absolute_position_test
    PRIVATE
        horizon
)

add_executable(multiwindow
    examples/multiwindow/main.cpp
)

target_link_libraries(multiwindow
    PRIVATE
        horizon
)

add_subdirectory(examples/launchpad)

add_executable(combo_demo
    examples/combo_demo/main.cpp
)

target_link_libraries(combo_demo
    PRIVATE
        horizon
)

add_executable(dialog_demo
    examples/dialog_demo.cpp
)

target_link_libraries(dialog_demo
    PRIVATE
        horizon
)

add_executable(min_size_test
    examples/min_size_test.cpp
)

target_link_libraries(min_size_test
    PRIVATE
        horizon
)

add_executable(treeview_demo
    examples/treeview_demo/main.cpp
)

target_link_libraries(treeview_demo
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