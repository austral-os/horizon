cmake_minimum_required(VERSION 3.10)
project(horizon-remote-storage)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)

add_library(horizon-remote-storage SHARED
    src/RemoteManager.cpp
)

target_include_directories(horizon-remote-storage
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${GIO_INCLUDE_DIRS}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/../../src
)

target_link_libraries(horizon-remote-storage
    PUBLIC
        horizon
        ${GIO_LIBRARIES}
)

install(TARGETS horizon-remote-storage
    DESTINATION lib
)
