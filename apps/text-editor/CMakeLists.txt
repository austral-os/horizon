cmake_minimum_required(VERSION 3.10)
project(text-editor)

find_package(PkgConfig REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(text-editor ${SOURCES})

target_include_directories(text-editor
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
)

target_link_libraries(text-editor
    PRIVATE
        horizon
        horizon-text
)

# Copiar archivos de локаles al directorio de compilación
add_custom_command(TARGET text-editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    ${CMAKE_CURRENT_BINARY_DIR}/locales
    COMMENT "Copying app locales to build directory"
)
