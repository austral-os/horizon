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
