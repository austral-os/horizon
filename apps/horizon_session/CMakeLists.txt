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
