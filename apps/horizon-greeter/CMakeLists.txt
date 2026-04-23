project(horizon-greeter)

set(SOURCES
    main.cpp
    GreetdClient.cpp
    UserProvider.cpp
    SessionProvider.cpp
    GreeterWindow.cpp
)

add_executable(horizon-greeter ${SOURCES})

target_link_libraries(horizon-greeter
    PRIVATE
        horizon
)

horizon_install_app(horizon-greeter
    APP_ID "horizon-greeter"
    NAME "Horizon Greeter"
    COMMENT "System Greeter Application"
    ICON "avatar-default"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
