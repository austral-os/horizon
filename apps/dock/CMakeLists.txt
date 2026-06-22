add_executable(dock
    src/main.cpp
    src/DockApplication.cpp
    src/DockItem.cpp
    src/DockShelf.cpp
)

target_include_directories(dock
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(dock PRIVATE
    HORIZON_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(dock
    PRIVATE
        horizon
)

add_custom_command(TARGET dock POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:dock>/locales
)

horizon_install_app(dock
    APP_ID "dock"
    NAME "Horizon Dock"
    COMMENT "System Dock"
    ICON "slingscold"
    VERSION "0.3.0"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
