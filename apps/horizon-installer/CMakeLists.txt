cmake_minimum_required(VERSION 3.10)
project(horizon-installer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(horizon-installer
    main.cpp
    Wizard.cpp
    InstallerPages.cpp
)

target_link_libraries(horizon-installer
    PRIVATE
        horizon
        horizon-disk-utilities
        horizon-installer-utils
        nlohmann_json::nlohmann_json
)

add_custom_command(TARGET horizon-installer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:horizon-installer>/locales
)

install(TARGETS horizon-installer
    DESTINATION bin
)

install(DIRECTORY locales/
    DESTINATION share/horizon/apps/horizon-installer/locales
)
