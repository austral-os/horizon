add_executable(horizon-powerd
    src/main.cpp
)

target_link_libraries(horizon-powerd
    PRIVATE
        horizon
)

horizon_install_app(horizon-powerd
    APP_ID "horizon-powerd"
    NAME "Horizon Power Daemon"
    COMMENT "Manage power settings and battery limits"
    ICON "power-manager"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true;X-GNOME-Autostart-enabled=true"
)
