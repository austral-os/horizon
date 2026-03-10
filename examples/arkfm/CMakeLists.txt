add_executable(arkfm
    src/main.cpp
    src/ArkfmApplication.cpp
    src/ArkfmListView.cpp
    src/ArkfmIconView.cpp
    src/ArkfmCoverFlowView.cpp
    src/ArkfmSidebar.cpp
    src/ArkfmToolbar.cpp
    src/ArkfmView.cpp
    src/ArkfmWindow.cpp
    src/ArkfmIconProvider.cpp
)

target_include_directories(arkfm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(arkfm
    PRIVATE
        horizon
)
