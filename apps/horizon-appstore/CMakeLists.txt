cmake_minimum_required(VERSION 3.10)
project(horizon-appstore VERSION 1.0.0)

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

add_custom_command(TARGET horizon-appstore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:horizon-appstore>/locales
)

horizon_install_app(horizon-appstore
    APP_ID "horizon-appstore"
    NAME "AppStore"
    COMMENT "Encuentra e instala aplicaciones en tu sistema."
    ICON "system-software-install"
    VERSION "0.3.0"
    CATEGORIES "System;"
)
