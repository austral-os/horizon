set(LIBRARY_NAME horizon-image)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

add_library(${LIBRARY_NAME} SHARED ${SOURCES})

target_include_directories(${LIBRARY_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${LIBRARY_NAME}
    PUBLIC
        horizon
)

install(TARGETS horizon-image DESTINATION lib COMPONENT libhorizon-image)

install(DIRECTORY include/
    DESTINATION include
    COMPONENT libhorizon-image-dev
)
