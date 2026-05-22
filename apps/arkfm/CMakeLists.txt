add_executable(arkfm
    src/main.cpp
    src/ArkfmApplication.cpp
    src/ArkfmWindow.cpp
    src/ArkfmFileProvider.cpp
    src/dialogs/NewFolderDialog.cpp
    src/dialogs/PropertiesDialog.cpp
    src/dialogs/RenameDialog.cpp
    src/dialogs/GoToFolderDialog.cpp
    src/dialogs/ConnectToServerDialog.cpp
)

target_include_directories(arkfm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(arkfm
    PRIVATE
        horizon
        horizon-image
        horizon-network-storage
        horizon-compression-tools
)

horizon_install_app(arkfm
    APP_ID "arkfm"
    NAME "Ark"
    COMMENT "Ark file manager"
    ICON "arkfm"
    CATEGORIES "System;FileVideo;"
    MIMETYPE "inode/directory;"
    VERSION "0.2.17"
)
