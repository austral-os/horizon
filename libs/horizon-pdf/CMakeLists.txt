cmake_minimum_required(VERSION 3.10)
project(horizon-pdf)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-glib)
pkg_check_modules(CAIRO REQUIRED cairo)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-pdf SHARED ${SOURCES})

target_include_directories(horizon-pdf
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${POPPLER_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
)

target_link_libraries(horizon-pdf
    PUBLIC
        horizon
        ${POPPLER_LIBRARIES}
        ${CAIRO_LIBRARIES}
)

set_target_properties(horizon-pdf PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS horizon-pdf DESTINATION lib COMPONENT libhorizon-pdf)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-pdf-dev
)
