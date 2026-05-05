if(NOT TARGET horizon-video)
    message(WARNING "horizon-video target not found, zenit will not be built")
    return()
endif()

set(APP_NAME zenit)

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
        horizon-video
)

horizon_install_app(${APP_NAME}
    APP_ID "zenit"
    NAME "Zenit Video Player"
    COMMENT "Reproductor de video de alta fidelidad"
    ICON "multimedia-video-player"
    CATEGORIES "Video;Player;"
    MIMETYPE "video/mp4;video/x-matroska;video/webm;video/ogg;video/mpeg;video/quicktime;"
    EXEC_ARGS "%F"
)
