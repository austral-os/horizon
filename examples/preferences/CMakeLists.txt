add_executable(preferences
    src/main.cpp
    src/PreferencesWindow.cpp
    src/PreferencesToolbar.cpp
    src/ContentView.cpp
    src/ViewPanel.cpp
)

target_link_libraries(preferences
    PRIVATE
        horizon
)

target_include_directories(preferences
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
