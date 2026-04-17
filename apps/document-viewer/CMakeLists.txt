cmake_minimum_required(VERSION 3.10)
project(document-viewer)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

set(SOURCES
    src/main.cpp
    src/DocumentWindow.cpp
    src/DocumentToolbar.cpp
    src/PdfSidebar.cpp
    src/PdfTabContent.cpp
)

add_executable(document-viewer ${SOURCES})

target_include_directories(document-viewer
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/../../include
        ${PROJECT_SOURCE_DIR}/../../src
        ${PROJECT_SOURCE_DIR}/../../libs/horizon-pdf/include
        ${CAIRO_INCLUDE_DIRS}
)

target_link_libraries(document-viewer
    PRIVATE
        horizon
        horizon-pdf
        ${CAIRO_LIBRARIES}
)

horizon_install_app(document-viewer
    APP_ID "document-viewer"
    NAME "Visor de Documentos"
    COMMENT "Ver y leer documentos PDF"
    ICON "document-viewer"
    CATEGORIES "Office;Viewer;"
)
