add_executable(capture
    src/main.cpp
    src/CaptureWindow.cpp
)

target_include_directories(capture
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(capture
    PRIVATE
        horizon
        horizon-capture
)

horizon_install_app(capture
    APP_ID capture
    NAME "Capture"
    COMMENT "Screen Capture and Recording"
    ICON "screenrecorder"
    VERSION "0.1.0"
    CATEGORIES "Utility;AudioVideo;"
)

add_custom_command(TARGET capture POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    ${CMAKE_CURRENT_BINARY_DIR}/locales
    COMMENT "Copying app locales to build directory"
)
