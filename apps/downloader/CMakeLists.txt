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

horizon_install_app(downloader
    APP_ID "downloader"
    NAME "Gestor de Descargas"
    COMMENT "Administra y supervisa tus descargas de archivos"
    ICON "downloader"
    CATEGORIES "Network;Utility;"
    EXTRA_DESKTOP "Name[en]=Download Manager\nComment[en]=Manage and monitor your file downloads"
)
