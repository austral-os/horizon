find_package(PkgConfig REQUIRED)

pkg_check_modules(WPEWEBKIT REQUIRED wpe-webkit-2.0)
pkg_check_modules(WPE REQUIRED wpe-1.0 wpebackend-fdo-1.0)
pkg_check_modules(CAIRO REQUIRED cairo)

set(SOURCES
    src/WebView.cpp
)

add_library(horizon-web SHARED ${SOURCES})

target_include_directories(horizon-web
    PUBLIC
        include
        ${WPEWEBKIT_INCLUDE_DIRS}
        ${WPE_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
    PRIVATE
        ${CAIRO_INCLUDE_DIRS}
)

target_link_libraries(horizon-web
    PUBLIC
        horizon
        ${WPEWEBKIT_LIBRARIES}
        ${WPE_LIBRARIES}
    PRIVATE
        ${CAIRO_LIBRARIES}
)

install(TARGETS horizon-web DESTINATION lib COMPONENT libhorizon-web)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-web-dev
)
