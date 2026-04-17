add_executable(arkfm
    src/main.cpp
    src/ArkfmApplication.cpp
    src/ArkfmWindow.cpp
    src/ArkfmFileProvider.cpp
    src/dialogs/NewFolderDialog.cpp
    src/dialogs/PropertiesDialog.cpp
    src/dialogs/RenameDialog.cpp
)

target_include_directories(arkfm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(arkfm
    PRIVATE
        horizon
)

add_custom_command(TARGET arkfm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:arkfm>/locales
)

horizon_install_app(arkfm
    APP_ID "arkfm"
    NAME "Ark"
    COMMENT "Ark file manager"
    ICON "arkfm"
    CATEGORIES "System;"
)
