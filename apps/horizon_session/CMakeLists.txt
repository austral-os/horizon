add_executable(horizon_session
    main.cpp
    HorizonSession.cpp
    DesktopParser.cpp
    IpcServer.cpp
)

target_link_libraries(horizon_session
    PRIVATE
        horizon
)

horizon_install_app(horizon_session
    APP_ID "session"
    NAME "Horizon Session"
    COMMENT "System Session Manager"
    ICON "applications-system"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)

# --- Session Desktop Files and Startup Scripts ---

set(SESSION_DESKTOPS
    horizon-wayfire.desktop
    horizon-labwc.desktop
)

set(SESSION_SCRIPTS
    start-horizon-session-wayfire.sh
    start-horizon-session-labwc.sh
)

foreach(desktop IN LISTS SESSION_DESKTOPS)
    configure_file(
        data/${desktop}.in
        ${CMAKE_CURRENT_BINARY_DIR}/${desktop}
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${desktop}
        DESTINATION share/wayland-sessions
        COMPONENT session
    )
endforeach()

foreach(script IN LISTS SESSION_SCRIPTS)
    configure_file(
        data/${script}.in
        ${CMAKE_CURRENT_BINARY_DIR}/${script}
        @ONLY
    )
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script}
        DESTINATION bin
        COMPONENT session
    )
endforeach()
