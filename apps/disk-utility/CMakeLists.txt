cmake_minimum_required(VERSION 3.10)
project(disk-utility)

file(GLOB SOURCES "*.cpp")

add_executable(disk-utility ${SOURCES})

target_include_directories(disk-utility
    PRIVATE
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${PROJECT_SOURCE_DIR}/../../libs/horizon-disk-utilities/include
)

target_link_libraries(disk-utility
    PRIVATE
        horizon
        horizon-disk-utilities
)

# Standard installation paths for applications
install(TARGETS disk-utility DESTINATION bin)
