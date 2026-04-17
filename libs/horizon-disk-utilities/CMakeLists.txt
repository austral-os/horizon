cmake_minimum_required(VERSION 3.10)
project(horizon-disk-utilities)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MOUNT REQUIRED mount)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-disk-utilities SHARED ${SOURCES})

target_include_directories(horizon-disk-utilities
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${MOUNT_INCLUDE_DIRS}
)

target_link_libraries(horizon-disk-utilities
    PUBLIC
        horizon
        ${MOUNT_LIBRARIES}
)

set_target_properties(horizon-disk-utilities PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-disk-utilities DESTINATION lib COMPONENT core)
