add_executable(horizon-notifications
    src/main.cpp
    src/NotificationManager.cpp
    src/NotificationWindow.cpp
    src/DbusNotificationServer.cpp
    src/SystemObserver.cpp
    src/NotificationApplication.cpp
)

target_include_directories(horizon-notifications
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(horizon-notifications
    PRIVATE
        horizon
)

horizon_install_app(horizon-notifications
    APP_ID "horizon-notifications"
    NAME "Horizon Notifications"
    COMMENT "Notification daemon for Horizon"
    ICON "dialog-information"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
