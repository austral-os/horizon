add_executable(top_panel
    src/main.cpp
    src/TopPanelApplication.cpp
    src/TopPanelWidget.cpp
    src/TopPanelMenuBar.cpp
    src/IndicatorsContainer.cpp
    src/DateAndTimeIndicator.cpp
    src/NetworkIndicator.cpp
)

target_include_directories(top_panel
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(top_panel
    PRIVATE
        horizon
)

add_custom_command(TARGET top_panel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:top_panel>/locales
)

horizon_install_app(top_panel
    APP_ID "top-panel"
    NAME "Top Panel"
    COMMENT "System Top Panel"
    ICON "slingscold"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
