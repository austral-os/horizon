add_library(horizon-overview SHARED
    src/Overview.cpp
)

target_link_libraries(horizon-overview
    PUBLIC
        horizon
)

target_include_directories(horizon-overview
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(horizon-overview PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_custom_command(TARGET horizon-overview POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/locales
    $<TARGET_FILE_DIR:horizon-overview>/locales
)

install(TARGETS horizon-overview DESTINATION lib COMPONENT core)
