include(cmake/HorizonMacros.cmake)




# CPack Configuration
set(CPACK_PACKAGE_NAME "horizon-desktop")
set(CPACK_PACKAGE_VENDOR "Austral OS")
set(CPACK_PACKAGE_VERSION "${HORIZON_VERSION}")
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

# Conflict and replace gnome-keyring to avoid file conflicts
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "gnome-keyring")
set(CPACK_DEBIAN_PACKAGE_REPLACES "gnome-keyring")
set(CPACK_DEBIAN_PACKAGE_PROVIDES "secret-service, gnome-keyring")

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

set(LIB_COMPONENTS "libhorizon;libhorizon-dev;libhorizon-capture;libhorizon-capture-dev;libhorizon-compression-tools;libhorizon-compression-tools-dev;libhorizon-disk-utilities;libhorizon-disk-utilities-dev;libhorizon-download;libhorizon-download-dev;libhorizon-image;libhorizon-image-dev;libhorizon-installer-utils;libhorizon-installer-utils-dev;libhorizon-network;libhorizon-network-dev;libhorizon-network-storage;libhorizon-network-storage-dev;libhorizon-overview;libhorizon-overview-dev;libhorizon-pdf;libhorizon-pdf-dev;libhorizon-terminal;libhorizon-terminal-dev;libhorizon-text;libhorizon-text-dev;libhorizon-video;libhorizon-video-dev;libhorizon-web;libhorizon-web-dev")

file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/CPackConfigLibs.cmake
    CONTENT "
include(\"${CMAKE_BINARY_DIR}/CPackConfig.cmake\")
set(CPACK_COMPONENTS_ALL \"${LIB_COMPONENTS}\")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE OFF)
set(CPACK_COMPONENTS_GROUPING \"IGNORE\")
set(CPACK_DEBIAN_FILE_NAME \"DEB-DEFAULT\")

# Unset gnome-keyring conflicts for libraries to avoid self-conflicts
set(CPACK_DEBIAN_PACKAGE_CONFLICTS \"\")
set(CPACK_DEBIAN_PACKAGE_REPLACES \"\")
set(CPACK_DEBIAN_PACKAGE_PROVIDES \"\")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA \"\")

foreach(comp \${CPACK_COMPONENTS_ALL})
    string(TOUPPER \"\${comp}\" comp_upper)
    set(CPACK_DEBIAN_\${comp_upper}_PACKAGE_NAME \"\${comp}\")
    set(CPACK_DEBIAN_\${comp_upper}_FILE_NAME \"\${comp}_\${CPACK_PACKAGE_VERSION}.deb\")
endforeach()
"
)

add_custom_target(package-libs
    COMMAND ${CMAKE_CPACK_COMMAND} --config ${CMAKE_BINARY_DIR}/CPackConfigLibs.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating individual .deb packages for libraries (runtime and -dev)..."
)
