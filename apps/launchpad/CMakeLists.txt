set(SOURCES
    src/main.cpp
    src/LaunchpadApplication.cpp
    src/LaunchpadWindow.cpp
    src/AppItem.cpp
)

add_executable(launchpad ${SOURCES})

target_include_directories(launchpad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(launchpad PRIVATE
    horizon
)

add_custom_command(TARGET launchpad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:launchpad>/locales
)
