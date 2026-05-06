find_library(LIBPROC2 NAMES proc2 REQUIRED)

add_executable(system-monitor
    main.cpp
    SystemMonitorWindow.cpp
    ProcessManager.cpp
)

target_link_libraries(system-monitor
    PRIVATE
        horizon
        ${LIBPROC2}
)

# Copy locales if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/locales)
    add_custom_command(TARGET system-monitor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/locales
        $<TARGET_FILE_DIR:system-monitor>/locales
    )
endif()

horizon_install_app(system-monitor
    APP_ID "system-monitor"
    NAME "System Monitor"
    COMMENT "Monitor system resources and processes"
    ICON "utilities-system-monitor"
    CATEGORIES "System;Monitor;"
)
