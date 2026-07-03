cmake_minimum_required(VERSION 3.10)
project(Horizon VERSION 0.5.75)
set(HORIZON_VERSION ${PROJECT_VERSION})

# Default install prefix to /usr if not specified
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Default install prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# RPATH configuration
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor wayland-egl)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(RSVG REQUIRED librsvg-2.0)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLESV2 REQUIRED glesv2)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
pkg_check_modules(MOUNT REQUIRED mount)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
set(PIPEWIRE_INCLUDE_DIRS ${PIPEWIRE_INCLUDE_DIRS} CACHE INTERNAL "")
set(PIPEWIRE_LIBRARIES ${PIPEWIRE_LIBRARIES} CACHE INTERNAL "")
message(STATUS "ROOT: PIPEWIRE_INCLUDE_DIRS: ${PIPEWIRE_INCLUDE_DIRS}")
include_directories(PkgConfig::PIPEWIRE) # This is rare but PkgConfig::PIPEWIRE is a target
# We'll use target_link_libraries properly instead

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c"
    "src/core/*.cpp"
    "src/core/ConfigManager.cpp"
    "src/core/MouseSettings.cpp"
    "src/core/FileWatcher.cpp"
    "src/core/lens/*.cpp"
    "src/files/disks/*.cpp"
    "src/files/dialogs/*.cpp"
    "src/files/FileProvider.cpp"
    "src/files/FileContextMenuBuilder.cpp"
    "src/protocols/*.c"
)
add_subdirectory(libs/horizon-print)

add_library(horizon SHARED ${SOURCES})
set_target_properties(horizon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Install Core Library
install(TARGETS horizon
    DESTINATION lib
    COMPONENT libhorizon
)

# Install Core Headers
install(DIRECTORY include/horizon/
    DESTINATION include/horizon
    COMPONENT libhorizon-dev
)

# Install Protocol Headers
install(DIRECTORY src/protocols/
    DESTINATION include/horizon/protocols
    COMPONENT libhorizon-dev
    FILES_MATCHING PATTERN "*.h"
)

# Configure and Install pkg-config file
configure_file(cmake/horizon.pc.in ${CMAKE_BINARY_DIR}/horizon.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/horizon.pc
    DESTINATION lib/pkgconfig
    COMPONENT libhorizon-dev
)

# Install Core Locales
install(DIRECTORY share/locales/
    DESTINATION share/horizon/locales
    COMPONENT libhorizon
)


# Headers públicos de tu librería
target_include_directories(horizon
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${DBUS_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
        ${PIPEWIRE_INCLUDE_DIRS}
        ${MOUNT_INCLUDE_DIRS}
        ${GIO_INCLUDE_DIRS}
    PRIVATE
        ${WAYLAND_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
        ${RSVG_INCLUDE_DIRS}
        ${EGL_INCLUDE_DIRS}
        ${GLESV2_INCLUDE_DIRS}
        ${FONTCONFIG_INCLUDE_DIRS}
)

target_compile_definitions(horizon
    PUBLIC
        HORIZON_CORE_VERSION="${HORIZON_VERSION}"
    PRIVATE
        HORIZON_BUILD_DIR="${CMAKE_BINARY_DIR}"
        HORIZON_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(horizon
    PUBLIC
        ${DBUS_LIBRARIES}
        horizon-print
        ${UUID_LIBRARIES}
    PRIVATE
        ${WAYLAND_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
        ${nlohmann_json_LIBRARIES}
        ${RSVG_LIBRARIES}
        ${EGL_LIBRARIES}
        ${GLESV2_LIBRARIES}
        ${FONTCONFIG_LIBRARIES}
        ${MOUNT_LIBRARIES}
        ${GIO_LIBRARIES}
)


# --- Macros and Functions ---
include(cmake/HorizonMacros.cmake)

# --- Libraries ---
add_subdirectory(libs/horizon-overview)
add_subdirectory(libs/horizon-terminal)
add_subdirectory(libs/horizon-web)
add_subdirectory(libs/horizon-pdf)
add_subdirectory(libs/horizon-image)
add_subdirectory(libs/horizon-video)
add_subdirectory(libs/horizon-text)
add_subdirectory(libs/horizon-disk-utilities)
add_subdirectory(libs/horizon-installer-utils)
add_subdirectory(libs/horizon-download)
add_subdirectory(libs/horizon-network)
add_subdirectory(libs/horizon-network-storage)
add_subdirectory(libs/horizon-compression-tools)
add_subdirectory(libs/horizon-capture)
add_subdirectory(libs/horizon-apt)

# --- Core Applications ---
add_subdirectory(apps/top_panel)
add_subdirectory(apps/horizon-notifications)
add_subdirectory(apps/horizon-polkit-agent)
add_subdirectory(apps/dock)
add_subdirectory(apps/horizon_wall)
add_subdirectory(apps/horizon_session)
add_subdirectory(apps/horizon-powerd)
add_subdirectory(apps/horizon-lock)
add_subdirectory(apps/aboutus)
add_subdirectory(apps/arkfm)
add_subdirectory(apps/launchpad)
add_subdirectory(apps/preferences)
add_subdirectory(apps/terminal)
add_subdirectory(apps/nova)
add_subdirectory(apps/document-viewer)
add_subdirectory(apps/image-viewer)
add_subdirectory(apps/text-editor)
add_subdirectory(apps/disk-utility)
add_subdirectory(apps/horizon-installer)
add_subdirectory(apps/horizon-greeter)
add_subdirectory(apps/austral-welcome)
add_subdirectory(apps/downloader)
add_subdirectory(apps/capture)
add_subdirectory(apps/zenit)
add_subdirectory(apps/calculator)
add_subdirectory(apps/system-monitor)
add_subdirectory(apps/horizon-keyring)
add_subdirectory(apps/horizon-keyring-manager)
add_subdirectory(apps/pkg-installer)
add_subdirectory(apps/horizon-lens)
add_subdirectory(apps/xdg-desktop-portal-horizon)
add_subdirectory(apps/horizon-appstore)
add_subdirectory(apps/horizon-splash)


# --- Examples and Demos ---
add_executable(minimal
    examples/minimal/main.cpp
)
target_link_libraries(minimal PRIVATE horizon)

add_executable(main_window
    examples/main_window/main_window.cpp
)
target_link_libraries(main_window PRIVATE horizon)

add_executable(mount_password_demo
    examples/mount_password_demo.cpp
)
target_link_libraries(mount_password_demo PRIVATE horizon horizon-network-storage)

# --- Demos and Tests ---
add_executable(files_demo
    examples/files-demo/FilesDemo.cpp
)
target_link_libraries(files_demo PRIVATE horizon)

add_executable(mouse_wheel_test
    tests/mouse_wheel_test.cpp
)
target_link_libraries(mouse_wheel_test PRIVATE horizon)

add_executable(absolute_position_test
    tests/absolute_position_test.cpp
)
target_link_libraries(absolute_position_test PRIVATE horizon)

add_executable(multiwindow
    examples/multiwindow/main.cpp
)
target_link_libraries(multiwindow PRIVATE horizon)



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

add_executable(file_picker_demo
    examples/file_picker_demo.cpp
)

target_link_libraries(file_picker_demo
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

add_executable(grouped_icons_demo
    examples/grouped_icons_demo/main.cpp
)

target_link_libraries(grouped_icons_demo
    PRIVATE
        horizon
)

add_executable(vault_demo
    examples/vault_demo/main.cpp
)

target_link_libraries(vault_demo
    PRIVATE
        horizon
)

add_executable(avatar_selector_demo
    examples/avatar_selector_demo/main.cpp
)

target_link_libraries(avatar_selector_demo
    PRIVATE
        horizon
)

target_compile_definitions(avatar_selector_demo PRIVATE 
    HORIZON_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_executable(notification_demo
    examples/notification_demo.cpp
)

target_link_libraries(notification_demo
    PRIVATE
        horizon
)

add_executable(test_notification_sender
    examples/test_notification_sender.cpp
)

target_link_libraries(test_notification_sender
    PRIVATE
        horizon
)

add_executable(notification_gui_demo
    examples/notification_gui_demo.cpp
)

target_link_libraries(notification_gui_demo
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

add_custom_target(run_notification
    COMMAND $<TARGET_FILE:notification_demo>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS notification_demo
    COMMENT "Ejecutando la demo de notificaciones..."
)

add_executable(i18n_example
    examples/i18n_example.cpp
)

target_link_libraries(i18n_example
    PRIVATE
        horizon
)
add_executable(i18n_demo
    examples/i18n_demo.cpp
)

target_link_libraries(i18n_demo
    PRIVATE
        horizon
)

add_executable(test_dlg_preferences
    examples/test_dlg_preferences.cpp
)

target_link_libraries(test_dlg_preferences
    PRIVATE
        horizon
)

add_executable(font_dialog_demo
    examples/font_dialog_demo.cpp
)

target_link_libraries(font_dialog_demo
    PRIVATE
        horizon
)

add_executable(aboutus_dialog_demo
    examples/aboutus_dialog_demo.cpp
)

target_link_libraries(aboutus_dialog_demo
    PRIVATE
        horizon
)

target_link_libraries(aboutus_dialog_demo
    PRIVATE
        horizon
)

add_executable(color_picker_demo
    examples/color_picker_demo.cpp
)

target_link_libraries(color_picker_demo
    PRIVATE
        horizon
)

add_executable(file_ops_demo
    examples/file_ops_demo.cpp
)

target_link_libraries(file_ops_demo
    PRIVATE
        horizon
)

add_executable(transparent_window_demo
    examples/transparent_window_demo.cpp
)

target_link_libraries(transparent_window_demo
    PRIVATE
        horizon
)

add_executable(ribbon_demo
    examples/ribbon_demo/main.cpp
)

target_link_libraries(ribbon_demo
    PRIVATE
        horizon
)


add_custom_command(TARGET i18n_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/i18n_demo/locales
    ${CMAKE_CURRENT_BINARY_DIR}/locales
    COMMENT "Copying demo locales to build directory"
)

# --- Tests ---
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()

    add_executable(events_manager_test tests/events_manager_test.cpp)
    target_link_libraries(events_manager_test PRIVATE horizon)
    add_test(NAME events_manager_test COMMAND events_manager_test)
    set_tests_properties(events_manager_test PROPERTIES TIMEOUT 10)
endif()

# --- Packaging ---
include(cmake/HorizonPackaging.cmake)
