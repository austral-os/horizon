set(SOURCES
    src/main.cpp
    src/BrowserWindow.cpp
)

add_executable(nova ${SOURCES})

target_include_directories(nova
    PRIVATE
        include
        ${PROJECT_SOURCE_DIR}/libs/horizon-web/include
)

target_link_libraries(nova
    PRIVATE
        horizon
        horizon-web
)
