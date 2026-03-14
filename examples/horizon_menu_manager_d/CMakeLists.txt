set(SOURCES
    main.cpp
    MenuManagerApplication.cpp
)

add_executable(horizon_menu_manager_d ${SOURCES})

target_include_directories(horizon_menu_manager_d PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(horizon_menu_manager_d PRIVATE
    horizon
)
