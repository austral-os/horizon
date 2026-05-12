find_library(LIBPROC2 NAMES proc2 REQUIRED)

add_executable(system-monitor
    main.cpp
    SystemMonitorWindow.cpp
    ProcessManager.cpp
    CPUStats.cpp
    MemoryStats.cpp
    EnergyStats.cpp
    DiskStats.cpp
    NetworkStats.cpp
)

target_link_libraries(system-monitor
    PRIVATE
        horizon
        ${LIBPROC2}
)

# horizon_install_app handles binary installation, locales, and desktop file generation
horizon_install_app(system-monitor
    APP_ID "system-monitor"
    NAME "System Monitor"
    COMMENT "Monitor system resources and processes"
    ICON "utilities-system-monitor"
    VERSION "0.2.0"
    CATEGORIES "System;Monitor;"
)
