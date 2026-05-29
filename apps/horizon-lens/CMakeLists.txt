add_executable(horizon-lens
    src/main.cpp
    src/LensService.cpp
    src/LensDbusService.cpp
    src/DirectoryScanner.cpp
    src/ThumbWorker.cpp
    src/ThumbGenerator.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER   REQUIRED poppler-glib)
pkg_check_modules(CAIRO     REQUIRED cairo)
pkg_check_modules(RSVG      REQUIRED librsvg-2.0)
pkg_check_modules(GLIB      REQUIRED glib-2.0)

find_package(nlohmann_json REQUIRED)

target_include_directories(horizon-lens
    PRIVATE
        ${POPPLER_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${RSVG_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(horizon-lens
    PRIVATE
        horizon
        ${POPPLER_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${RSVG_LIBRARIES}
        ${GLIB_LIBRARIES}
        nlohmann_json::nlohmann_json
        pthread
)

target_compile_definitions(horizon-lens PRIVATE
    # No extra defines needed here
)

horizon_install_app(horizon-lens
    APP_ID  "horizon-lens"
    NAME    "Horizon Lens"
    COMMENT "Background thumbnail generation service for Horizon OS"
    ICON    "image-x-generic"
    VERSION "0.1.1"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true;X-GNOME-Autostart-enabled=true"
)
