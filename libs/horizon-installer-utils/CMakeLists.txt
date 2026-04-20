cmake_minimum_required(VERSION 3.10)
project(horizon-installer-utils)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(UUID REQUIRED uuid)

add_library(horizon-installer-utils SHARED
    src/InstallerManager.cpp
)

target_include_directories(horizon-installer-utils
    PUBLIC
        include
        ${PROJECT_SOURCE_DIR}/../../include
        ${DBUS_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/../../src
)

target_link_libraries(horizon-installer-utils
    PUBLIC
        horizon
        horizon-disk-utilities
        ${DBUS_LIBRARIES}
        ${UUID_LIBRARIES}
)

install(TARGETS horizon-installer-utils
    DESTINATION lib
    COMPONENT core
)

install(DIRECTORY include/horizon-installer-utils
    DESTINATION include
    COMPONENT dev
)
