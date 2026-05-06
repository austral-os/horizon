add_executable(horizon-lock
    src/main.cpp
)

target_link_libraries(horizon-lock
    PRIVATE
        horizon
        dl
)

horizon_install_app(horizon-lock
    APP_ID "horizon-lock"
    NAME "Horizon Lock Screen"
    COMMENT "Lock your session"
    ICON "system-lock-screen"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
