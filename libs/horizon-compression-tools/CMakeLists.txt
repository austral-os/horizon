set(SOURCES
    src/CompressionTask.cpp
    src/CompressionManager.cpp
)

add_library(horizon-compression-tools SHARED ${SOURCES})

target_include_directories(horizon-compression-tools
    PUBLIC
        include
)

target_link_libraries(horizon-compression-tools
    PUBLIC
        horizon
)

install(TARGETS horizon-compression-tools DESTINATION lib COMPONENT core)
