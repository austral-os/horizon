add_executable(preferences
    src/main.cpp
    src/PreferencesWindow.cpp
    src/PreferencesToolbar.cpp
    src/ContentView.cpp
    src/ViewPanel.cpp
    src/ImagesView.cpp
    src/views/DesktopView/DesktopView.cpp
    src/views/DesktopView/WallpaperView.cpp
    src/views/DesktopView/DockView.cpp
    src/views/AppearanceView/AppearanceView.cpp
    src/views/CompositorView/CompositorView.cpp
    src/views/NotificationsView/NotificationsView.cpp
    src/views/DisplayView/DisplayView.cpp
    src/views/DisplayView/DisplayDevices.cpp
    src/views/DisplayView/LabwcAdapter.cpp
    src/views/DisplayView/KwinAdapter.cpp
    src/views/DisplayView/MeteorAdapter.cpp
    src/views/SoundView/SoundView.cpp
    src/views/MouseView/MouseView.cpp
    src/views/KeyboardView/KeyboardView.cpp
    src/views/KeyboardView/KeyboardHardwareView.cpp
    src/views/PrintersView/PrintersView.cpp
    src/views/PrintersView/AddPrinterDialog.cpp
    src/views/PrintersView/DriverSearchDialog.cpp
    src/views/PowerView/PowerView.cpp
    src/views/WifiView/WifiView.cpp
    src/views/WifiView/WifiConfigView.cpp
    src/views/WifiView/WifiConnectDialog.cpp
    src/views/BluetoothView/BluetoothView.cpp
    src/views/BluetoothView/BluetoothPairDialog.cpp
    src/views/NetworkView/NetworkView.cpp
    src/views/NetworkView/AdvancedNetworkView.cpp
    src/views/UsersView/UsersView.cpp

    src/views/UsersView/PasswordDialog.cpp
    src/views/DateTimeView/DateTimeView.cpp
    src/views/DateTimeView/TimeZonePickerDialog.cpp
    src/views/RegionView/RegionView.cpp
    src/views/DetailsView/DetailsView.cpp
    src/views/ApplicationsView/ApplicationsView.cpp
    src/views/ApplicationsView/MimeTypesView.cpp
    src/views/ApplicationsView/InputDialog.cpp
    src/views/ApplicationsView/DefaultAppsView.cpp
    src/views/ApplicationsView/StartupAppsView.cpp
    src/views/ApplicationsView/StartupEditDialog.cpp
    src/utils/XkbParser.cpp
    src/utils/MeteorConfigWriter.cpp
    src/utils/MeteorPluginManager.cpp
    src/views/KeyboardView/KeyboardLanguageView.cpp
    src/views/ApplicationsView/LayoutPickerDialog.cpp
    src/utils/PipeWireManager.cpp
)

target_link_libraries(preferences
    PRIVATE
        horizon
        horizon-overview
        horizon-network
        horizon-print
        PkgConfig::PIPEWIRE
        nlohmann_json::nlohmann_json

)

add_custom_command(TARGET preferences POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:preferences>/locales
)

horizon_install_app(preferences
    APP_ID "preferences"
    NAME "Preferences"
    COMMENT "Horizon System Preferences"
    ICON "applications-system"
    VERSION "0.1.7"
    CATEGORIES "System;"
)

target_include_directories(preferences
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PIPEWIRE_INCLUDE_DIRS}
)

