set(SOURCES
    src/main.cpp
    src/DownloaderWindow.cpp
    src/NewDownloadDialog.cpp
)

add_executable(downloader ${SOURCES})

target_include_directories(downloader
    PRIVATE
        include
)

target_link_libraries(downloader
    PRIVATE
        horizon
        horizon-download
)

install(TARGETS downloader DESTINATION bin COMPONENT apps)
