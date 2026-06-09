cmake_minimum_required(VERSION 3.10)
project(horizon-appstore)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "*.cpp" "views/*.cpp")

add_executable(horizon-appstore ${SOURCES})

target_include_directories(horizon-appstore PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/views
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs/horizon-apt/include
)

target_link_libraries(horizon-appstore PUBLIC
    horizon
    horizon-apt
)

install(TARGETS horizon-appstore DESTINATION bin)
