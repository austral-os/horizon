find_package(PkgConfig REQUIRED)
pkg_check_modules(SOUP REQUIRED libsoup-3.0)

set(SOURCES
    src/DownloadTask.cpp
    src/DownloadManager.cpp
    src/DownloadItemWidget.cpp
    src/DownloadView.cpp
)

add_library(horizon-download SHARED ${SOURCES})

target_include_directories(horizon-download
    PUBLIC
        include
        ${SOUP_INCLUDE_DIRS}
)

target_link_libraries(horizon-download
    PUBLIC
        horizon
        ${SOUP_LIBRARIES}
)

install(TARGETS horizon-download DESTINATION lib COMPONENT core)
