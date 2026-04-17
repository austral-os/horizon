cmake_minimum_required(VERSION 3.10)
project(horizon-terminal)

find_package(PkgConfig REQUIRED)
pkg_check_modules(VTERM REQUIRED vterm)

pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HB REQUIRED harfbuzz)
pkg_check_modules(FC REQUIRED fontconfig)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-terminal SHARED ${SOURCES})

target_include_directories(horizon-terminal
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${VTERM_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${FREETYPE_INCLUDE_DIRS}
        ${HB_INCLUDE_DIRS}
        ${FC_INCLUDE_DIRS}
)

target_link_libraries(horizon-terminal
    PUBLIC
        horizon
        ${VTERM_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${FREETYPE_LIBRARIES}
        ${HB_LIBRARIES}
        ${FC_LIBRARIES}
        util
)

set_target_properties(horizon-terminal PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-terminal DESTINATION lib COMPONENT core)
