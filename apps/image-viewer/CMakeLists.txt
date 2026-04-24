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

horizon_install_app(${APP_NAME}
    APP_ID "image-viewer"
    NAME "Visor de Imágenes"
    COMMENT "Ver y rotar imágenes"
    ICON "image-viewer"
    CATEGORIES "Graphics;Viewer;"
    MIMETYPE "image/bmp;image/gif;image/jpeg;image/jpg;image/pjpeg;image/png;image/tiff;image/x-bmp;image/x-pcx;image/x-png;image/x-portable-anymap;image/x-portable-bitmap;image/x-portable-graymap;image/x-portable-pixmap;image/x-tga;image/x-xbitmap;"
    EXEC_ARGS "%F"
)
