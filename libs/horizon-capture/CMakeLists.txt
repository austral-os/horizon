project(horizon-capture)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

set(SOURCES 
    src/CaptureEngine.cpp
    src/SelectionWidget.cpp
    src/SelectionWindow.cpp
    src/VideoRecorder.cpp
    ../../src/protocols/wlr-screencopy-unstable-v1-protocol.c
)

message(STATUS "CAPTURE SOURCES: ${SOURCES}")

add_library(horizon-capture SHARED ${SOURCES})

target_include_directories(horizon-capture
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
    PRIVATE
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
        ${SWSCALE_INCLUDE_DIRS}
)

target_link_libraries(horizon-capture
    PUBLIC
        horizon
        wayland-client
        cairo
        png
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
)

install(TARGETS horizon-capture DESTINATION lib COMPONENT core)
