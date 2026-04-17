add_executable(aboutus
    main.cpp
    AboutWindow.cpp
)

target_link_libraries(aboutus
    PRIVATE
        horizon
        horizon-overview
)

add_custom_command(TARGET aboutus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:aboutus>/locales
)

horizon_install_app(aboutus
    APP_ID "aboutus"
    NAME "About System"
    COMMENT "About system info"
    ICON "computer"
    CATEGORIES "System;"
)
