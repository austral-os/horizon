cmake_minimum_required(VERSION 3.16)
project(horizon-print VERSION 0.1.0)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(horizon-print SHARED ${SOURCES})
set_target_properties(horizon-print PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(horizon-print PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(horizon-print PUBLIC cxx_std_20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CUPS REQUIRED cups)

target_include_directories(horizon-print PRIVATE ${CUPS_INCLUDE_DIRS})
target_link_libraries(horizon-print PUBLIC horizon ${CUPS_LIBRARIES})

install(TARGETS horizon-print DESTINATION lib COMPONENT libhorizon-print)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-print-dev
)
