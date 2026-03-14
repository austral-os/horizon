set(SOURCES
    main.cpp
    WallApplication.cpp
)

add_executable(horizon_wall ${SOURCES})

target_include_directories(horizon_wall PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(horizon_wall PRIVATE
    horizon
)
