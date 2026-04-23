add_executable(horizon_session
    main.cpp
    HorizonSession.cpp
    DesktopParser.cpp
    IpcServer.cpp
)

target_compile_definitions(horizon_session PRIVATE 
    HORIZON_BUILD_BIN_DIR="${CMAKE_BINARY_DIR}"
    HORIZON_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(horizon_session
    PRIVATE
        horizon
        horizon-installer-utils
)

horizon_install_app(horizon_session
    APP_ID "session"
    NAME "Horizon Session"
    COMMENT "System Session Manager"
    ICON "applications-system"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)

# --- Session Desktop Files and Startup Scripts ---

set(SESSION_DESKTOPS
    horizon-wayfire.desktop
    horizon-labwc.desktop
)

set(SESSION_SCRIPTS
    start-horizon-session-wayfire.sh
    start-horizon-session-labwc.sh
)

foreach(desktop IN LISTS SESSION_DESKTOPS)
    configure_file(
        data/${desktop}.in
        ${CMAKE_CURRENT_BINARY_DIR}/${desktop}
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${desktop}
        DESTINATION share/wayland-sessions
        COMPONENT session
    )
endforeach()

foreach(script IN LISTS SESSION_SCRIPTS)
    configure_file(
        data/${script}.in
        ${CMAKE_CURRENT_BINARY_DIR}/${script}
        @ONLY
    )
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script}
        DESTINATION bin
        COMPONENT session
    )
endforeach()

# --- Fonts ---

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/fonts")
    install(DIRECTORY data/fonts/
        DESTINATION share/fonts/horizon
        COMPONENT session
    )
endif()

# --- Icon Theme ---

set(ICON_TARBALL "data/austral-icon-theme.tar.gz")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ICON_TARBALL}")
    set(EXTRACT_DIR "${CMAKE_CURRENT_BINARY_DIR}/icons_extracted")
    file(MAKE_DIRECTORY "${EXTRACT_DIR}")
    
    # Extract at build time
    add_custom_command(
        OUTPUT "${EXTRACT_DIR}/.extracted"
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_CURRENT_SOURCE_DIR}/${ICON_TARBALL}"
        COMMAND ${CMAKE_COMMAND} -E touch "${EXTRACT_DIR}/.extracted"
        WORKING_DIRECTORY "${EXTRACT_DIR}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${ICON_TARBALL}"
        COMMENT "Extracting Austral Icon Theme..."
    )
    
    add_custom_target(extract_icons ALL DEPENDS "${EXTRACT_DIR}/.extracted")

    # Install the contents (the 'Light' theme folder among others)
    # We install it specifically as "austral" so it can be looked up by that name
    install(DIRECTORY "${EXTRACT_DIR}/austral-icon-theme/Light/"
        DESTINATION share/icons/austral
        COMPONENT session
        PATTERN ".git" EXCLUDE
        PATTERN "README.md" EXCLUDE
    )
endif()

# --- Color Scheme ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/color-scheme.json")
    install(FILES data/color-scheme.json
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- Country Data ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/countries.json")
    install(FILES data/countries.json
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- Dock Config ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/dock.json")
    install(FILES data/dock.json
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- Desktop Config ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/desktop.json")
    install(FILES data/desktop.json
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- Terminal Config ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/terminal.json")
    install(FILES data/terminal.json
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- Text Editor Config ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/text-editor.json")
    install(FILES data/text-editor.json
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- License ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/license_lgplv3.txt")
    install(FILES data/license_lgplv3.txt
        DESTINATION share/horizon
        COMPONENT session
    )
endif()

# --- Backgrounds ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/backgrounds")
    install(DIRECTORY data/backgrounds/
        DESTINATION share/horizon/backgrounds
        COMPONENT session
    )
endif()
