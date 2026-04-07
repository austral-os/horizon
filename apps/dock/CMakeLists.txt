add_executable(dock
    src/main.cpp
    src/DockApplication.cpp
    src/DockItem.cpp
    src/DockShelf.cpp
)

target_include_directories(dock
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dock
    PRIVATE
        horizon
)
