add_executable(capture_app
    src/main.cpp
    src/CaptureWindow.cpp
)

set_target_properties(capture_app PROPERTIES OUTPUT_NAME horizon-capture)

target_include_directories(capture_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(capture_app
    PRIVATE
        horizon
        horizon-capture
)

horizon_install_app(capture_app
    APP_ID org.horizon.capture
    NAME "Capture"
    COMMENT "Screen Capture and Recording"
    ICON "screenrecorder"
    CATEGORIES "Utility;AudioVideo;"
)

add_custom_command(TARGET capture_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    ${CMAKE_CURRENT_BINARY_DIR}/locales
    COMMENT "Copying app locales to build directory"
)
