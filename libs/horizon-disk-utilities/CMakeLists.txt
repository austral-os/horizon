cmake_minimum_required(VERSION 3.10)
project(horizon-disk-utilities)

# This library is now integrated into horizon core.
# We keep this target as an INTERFACE for compatibility.

add_library(horizon-disk-utilities INTERFACE)

target_include_directories(horizon-disk-utilities
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
)

target_link_libraries(horizon-disk-utilities
    INTERFACE
        horizon
)

# No installation needed for an interface library that is just a proxy to horizon

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-disk-utilities-dev
)
