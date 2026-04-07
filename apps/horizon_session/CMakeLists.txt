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
