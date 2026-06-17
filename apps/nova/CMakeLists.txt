set(SOURCES
    src/main.cpp
    src/BrowserWindow.cpp
    src/NovaToolbar.cpp
    src/DownloadsPage.cpp
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
        horizon-download
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
    VERSION "0.1.2"
    CATEGORIES "Internet;"
    MIMETYPE "text/html;text/xml;application/xhtml+xml;application/xml;application/rss+xml;application/rdf+xml;x-scheme-handler/http;x-scheme-handler/https;"
    EXEC_ARGS "%U"
)
