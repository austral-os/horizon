set(LIBRARY_NAME horizon-video)

pkg_check_modules(MPV QUIET mpv)

if(NOT MPV_FOUND)
    message(WARNING "mpv not found, horizon-video will not be built")
    return()
endif()

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

add_library(${LIBRARY_NAME} SHARED ${SOURCES})

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
        ${MPV_INCLUDE_DIRS}
)

target_link_libraries(${LIBRARY_NAME}
    PUBLIC
        horizon
        ${MPV_LIBRARIES}
)

install(TARGETS horizon-video DESTINATION lib COMPONENT core)
