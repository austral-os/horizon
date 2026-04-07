add_executable(top_panel
    src/main.cpp
    src/TopPanelApplication.cpp
    src/TopPanelWidget.cpp
    src/TopPanelMenuBar.cpp
    src/IndicatorsContainer.cpp
    src/DateAndTimeIndicator.cpp
    src/NetworkIndicator.cpp
)

target_include_directories(top_panel
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(top_panel
    PRIVATE
        horizon
)
