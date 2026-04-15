set(APP_NAME image-viewer)

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
        horizon-image
)

install(TARGETS ${APP_NAME} DESTINATION bin)
