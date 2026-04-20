include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
# Function to install an app with its locales and desktop file
macro(horizon_install_app TARGET_NAME)
    set(options)
    set(oneValueArgs APP_ID NAME COMMENT ICON TERMINAL CATEGORIES EXTRA_DESKTOP)
    set(multiValueArgs)
    cmake_parse_arguments(APP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

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
        set(APP_CATEGORIES "Utility;")
    endif()

    # Install Binary
    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin
        COMPONENT ${APP_APP_ID}
    )

    # Normalize APP_ID to use hyphens for the package name
    string(REPLACE "_" "-" APP_ID_NORM "${APP_APP_ID}")
    
    # Set Debian package name for this component
    if(APP_ID_NORM MATCHES "^horizon-")
        set(CPACK_DEBIAN_${APP_APP_ID}_PACKAGE_NAME "${APP_ID_NORM}")
    else()
        set(CPACK_DEBIAN_${APP_APP_ID}_PACKAGE_NAME "horizon-${APP_ID_NORM}")
    endif()

    # Install Locales
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/locales")
        install(DIRECTORY locales/
            DESTINATION share/horizon/apps/${APP_APP_ID}/locales
            COMPONENT ${APP_APP_ID}
        )
    endif()

    # Generate and Install Desktop File
    set(APP_EXEC "${CMAKE_INSTALL_PREFIX}/bin/${TARGET_NAME}")
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

    if(COMMAND cpack_add_component)
        # Inter-component dependencies (using component names)
        set(COMPONENT_DEPS "core") 
        
        cpack_add_component(${APP_APP_ID}
            DISPLAY_NAME "${APP_NAME}"
            DESCRIPTION "${APP_COMMENT}"
            DEPENDS ${COMPONENT_DEPS}
        )

        # External Debian dependencies for this specific component
        if("${APP_APP_ID}" STREQUAL "session")
            set(CPACK_DEBIAN_SESSION_PACKAGE_DEPENDS "${HORIZON_RUNTIME_DEPENDS}")
        endif()
    endif()
endmacro()

# --- Runtime Dependencies (External Debian packages) ---
set(HORIZON_RUNTIME_DEPENDS
    "wayfire (>= 0.9.0), labwc (>= 0.8.3), wayland-utils (>= 1.2.0), wlr-randr (>= 0.4.1), xdg-utils (>= 1.2.1), shared-mime-info (>= 2.4), fontconfig (>= 2.15.0), librsvg2-common (>= 2.60.0), gstreamer1.0-plugins-bad (>= 1.26.2), gstreamer1.0-libav (>= 1.26.2), desktop-file-utils"
)

# CPack Configuration
set(CPACK_PACKAGE_NAME "horizon-desktop")
set(CPACK_PACKAGE_VENDOR "Austral OS")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Horizon Desktop Environment Applications")
set(CPACK_PACKAGE_CONTACT "Horacio <user@example.com>") # Placeholder, should be updated if known

# Configure scripts
configure_file(${CMAKE_SOURCE_DIR}/cmake/postinst.in ${CMAKE_BINARY_DIR}/postinst @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/cmake/prerm.in ${CMAKE_BINARY_DIR}/prerm @ONLY)

set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SET_DESTDIR ON)
set(CPACK_INSTALL_PREFIX "/usr")

# Debian specifics
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# Merging manual dependencies with shlibdeps for monolithic package
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${HORIZON_RUNTIME_DEPENDS}")

# Scripts
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/prerm")

# Allow switching between monolithic and component-based packaging
# Default to monolithic unless HORIZON_PACKAGING_COMPONENTS is ON
option(HORIZON_PACKAGING_COMPONENTS "Generate separate .deb for each component" OFF)

if(HORIZON_PACKAGING_COMPONENTS)
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE OFF)
    set(CPACK_COMPONENTS_GROUPING "IGNORE")
    set(CPACK_PACKAGE_FILE_NAME "horizon")
else()
    set(CPACK_DEB_COMPONENT_INSTALL OFF)
    set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
    set(CPACK_PACKAGE_FILE_NAME "horizon-desktop")
endif()

include(CPack)

# Native Ninja/Make targets for easy packaging
add_custom_target(package-monolithic
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB -D CPACK_DEB_COMPONENT_INSTALL=OFF -D CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE=ON
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating monolithic .deb package..."
)

add_custom_target(package-components
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB -D CPACK_DEB_COMPONENT_INSTALL=ON -D CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE=OFF -D CPACK_COMPONENTS_GROUPING=IGNORE
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating individual .deb packages for each application..."
)
