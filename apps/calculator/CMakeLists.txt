set(APP_NAME calculator)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

add_executable(${APP_NAME} ${SOURCES})

target_include_directories(${APP_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${APP_NAME}
    PRIVATE
        horizon
)

horizon_install_app(${APP_NAME}
    APP_ID "calculator"
    NAME "Calculator"
    COMMENT "Simple Calculator for Austral OS"
    ICON "multimedia-video-player"
    VERSION "0.1.1"
    CATEGORIES "Calculator;Utility;"
    EXEC_ARGS "%F"
)