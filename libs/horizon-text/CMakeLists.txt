cmake_minimum_required(VERSION 3.10)
project(horizon-text)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PANGO REQUIRED pango pangocairo)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-text SHARED ${SOURCES})

target_include_directories(horizon-text
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${PANGO_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${XKBCOMMON_INCLUDE_DIRS}
)

target_link_libraries(horizon-text
    PUBLIC
        horizon
        ${PANGO_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${XKBCOMMON_LIBRARIES}
)

set_target_properties(horizon-text PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-text DESTINATION lib COMPONENT libhorizon)
