set(SOURCES
    main.cpp
    WallApplication.cpp
)

add_executable(horizon_wall ${SOURCES})

target_include_directories(horizon_wall PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(horizon_wall PRIVATE
    horizon
)

horizon_install_app(horizon_wall
    APP_ID "wall"
    NAME "Horizon Wallpaper"
    COMMENT "System Wallpaper Manager"
    ICON "image-viewer"
    VERSION "0.1.1"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
