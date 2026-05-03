set(LIBRARY_NAME horizon-capture)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "../../src/protocols/wlr-screencopy-unstable-v1-protocol.c"
)

add_library(${LIBRARY_NAME} SHARED ${SOURCES})

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${LIBRARY_NAME}
    PUBLIC
        horizon
        wayland-client
        cairo
        png
)

install(TARGETS horizon-capture DESTINATION lib COMPONENT core)
