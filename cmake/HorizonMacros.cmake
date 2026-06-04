include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# --- Runtime Dependencies (External Debian packages) ---
set(HORIZON_RUNTIME_DEPENDS
    "meteor (>= 0.1.0), labwc (>= 0.8.3), wayland-utils (>= 1.2.0), wlr-randr (>= 0.4.1), xdg-utils (>= 1.2.1), xdg-desktop-portal, xdg-desktop-portal-gtk, shared-mime-info (>= 2.4), fontconfig (>= 2.15.0), librsvg2-common (>= 2.60.0), gstreamer1.0-plugins-bad (>= 1.26.2), gstreamer1.0-libav (>= 1.26.2), desktop-file-utils, libsqlite3-0, libssl3, libpam0g"
)

# Function to install an app with its locales and desktop file
macro(horizon_install_app TARGET_NAME)
    set(options)
    set(oneValueArgs APP_ID NAME COMMENT ICON TERMINAL EXEC_PREFIX EXEC_ARGS EXTRA_DESKTOP VERSION)
    set(multiValueArgs MIMETYPE CATEGORIES)
    cmake_parse_arguments(APP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT APP_VERSION)
        set(APP_VERSION ${HORIZON_VERSION})
    endif()

    if(NOT APP_APP_ID)
        set(APP_APP_ID ${TARGET_NAME})
    endif()
    if(NOT APP_NAME)
        set(APP_NAME ${TARGET_NAME})
    endif()
    if(NOT APP_COMMENT)
        set(APP_COMMENT "Horizon ${APP_NAME}")
    endif()
    if(NOT APP_ICON)
        set(APP_ICON "applications-other")
    endif()
    if(NOT APP_TERMINAL)
        set(APP_TERMINAL "false")
    endif()
    if(NOT APP_CATEGORIES)
        set(APP_CAT "Utility;")
    else()
        set(APP_CAT "")
        foreach(cat ${APP_CATEGORIES})
            if(NOT cat STREQUAL "")
                set(APP_CAT "${APP_CAT}${cat};")
            endif()
        endforeach()
    endif()

    # Install Binary
    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin
        COMPONENT ${APP_APP_ID}
    )

    # Pass app version as compile definition
    target_compile_definitions(${TARGET_NAME} PRIVATE APP_VERSION="${APP_VERSION}")

    # Normalize APP_ID to use hyphens for the package name
    string(REPLACE "_" "-" APP_ID_NORM "${APP_APP_ID}")
    
    # Store the component's specific version in a global property
    set_property(GLOBAL PROPERTY HORIZON_APP_VERSION_${APP_APP_ID} "${APP_VERSION}")

    # Get the actual output name of the binary
    get_target_property(APP_BINARY_NAME ${TARGET_NAME} OUTPUT_NAME)
    if(NOT APP_BINARY_NAME)
        set(APP_BINARY_NAME ${TARGET_NAME})
    endif()

    # Install Locales
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/locales")
        install(DIRECTORY locales/
            DESTINATION share/horizon/apps/${APP_APP_ID}/locales
            COMPONENT ${APP_APP_ID}
        )
        
        # Copy locales to build directory for local testing
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/locales
            $<TARGET_FILE_DIR:${TARGET_NAME}>/locales
            COMMENT "Copying app locales to build directory"
        )
    endif()

    # Generate and Install Desktop File
    if(APP_EXEC_PREFIX)
        set(APP_EXEC "${APP_EXEC_PREFIX} ${CMAKE_INSTALL_PREFIX}/bin/${APP_BINARY_NAME}")
    else()
        set(APP_EXEC "${CMAKE_INSTALL_PREFIX}/bin/${APP_BINARY_NAME}")
    endif()
    
    if(APP_EXEC_ARGS)
        set(APP_EXEC "${APP_EXEC} ${APP_EXEC_ARGS}")
    endif()
    set(APP_CATEGORIES "${APP_CAT}")
    
    if(APP_MIMETYPE)
        set(MIME_STR "")
        foreach(m ${APP_MIMETYPE})
            if(NOT m STREQUAL "")
                set(MIME_STR "${MIME_STR}${m};")
            endif()
        endforeach()
        set(APP_MIME "MimeType=${MIME_STR}")
    else()
        set(APP_MIME "")
    endif()
    set(APP_EXTRA "${APP_EXTRA_DESKTOP}")
    
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/app.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/${APP_APP_ID}.desktop
        @ONLY
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APP_APP_ID}.desktop
        DESTINATION share/applications
        COMPONENT ${APP_APP_ID}
    )

    # Track all app components globally so we can build a metapackage later
    set_property(GLOBAL APPEND PROPERTY HORIZON_APP_COMPONENTS ${APP_APP_ID})

endmacro()
