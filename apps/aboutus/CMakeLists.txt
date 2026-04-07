add_executable(aboutus
    main.cpp
    AboutWindow.cpp
)

target_link_libraries(aboutus
    PRIVATE
        horizon
        horizon-overview
)
