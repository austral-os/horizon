set(SOURCES
    src/main.cpp
    src/BrowserWindow.cpp
    src/NovaToolbar.cpp
)

add_executable(nova ${SOURCES})

target_include_directories(nova
    PRIVATE
        include
        ${PROJECT_SOURCE_DIR}/libs/horizon-web/include
)

target_link_libraries(nova
    PRIVATE
        horizon
        horizon-web
)

add_custom_command(TARGET nova POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:nova>/locales
)

horizon_install_app(nova
    APP_ID "nova"
    NAME "Nova"
    COMMENT "Nova Web Browser"
    ICON "nova-browser"
    CATEGORIES "Internet;"
)
