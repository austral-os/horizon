cmake_minimum_required(VERSION 3.10)
project(horizon-network)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(UUID REQUIRED uuid)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-network SHARED ${SOURCES})

target_include_directories(horizon-network
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${DBUS_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
)

target_link_libraries(horizon-network
    PUBLIC
        horizon
        ${DBUS_LIBRARIES}
        ${UUID_LIBRARIES}
)

set_target_properties(horizon-network PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-network DESTINATION lib COMPONENT core)
