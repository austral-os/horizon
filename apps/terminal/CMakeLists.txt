cmake_minimum_required(VERSION 3.10)
project(terminal)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
    ${PROJECT_SOURCE_DIR}/../../libs/horizon-terminal/include
)

add_executable(terminal ${SOURCES})

target_link_libraries(terminal
    PRIVATE
        horizon
        horizon-terminal
)

add_custom_command(TARGET terminal POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:terminal>/locales
)
