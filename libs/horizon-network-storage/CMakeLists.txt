cmake_minimum_required(VERSION 3.10)
project(horizon-network-storage)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)
pkg_check_modules(SECRET libsecret-1)

if(SECRET_FOUND)
    add_definitions(-DHAVE_LIBSECRET)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-network-storage SHARED ${SOURCES})

target_include_directories(horizon-network-storage
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${GIO_INCLUDE_DIRS}
        ${SECRET_INCLUDE_DIRS}
)

target_link_libraries(horizon-network-storage
    PUBLIC
        horizon
        ${GIO_LIBRARIES}
        ${SECRET_LIBRARIES}
)

set_target_properties(horizon-network-storage PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-network-storage DESTINATION lib COMPONENT libhorizon-network-storage)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-network-storage-dev
)
