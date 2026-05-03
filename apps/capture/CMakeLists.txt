add_executable(capture_app
    src/main.cpp
)

set_target_properties(capture_app PROPERTIES OUTPUT_NAME horizon-capture)

target_link_libraries(capture_app
    PRIVATE
        horizon
        horizon-capture
)

# No desktop file yet for MVP as it's CLI based
