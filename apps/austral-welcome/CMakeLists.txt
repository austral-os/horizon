cmake_minimum_required(VERSION 3.10)
project(austral-welcome)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(austral-welcome
    main.cpp
    WelcomeWindow.cpp
)

target_link_libraries(austral-welcome
    PRIVATE
        horizon
)

horizon_install_app(austral-welcome
    APP_ID "austral-welcome"
    NAME "Austral Welcome"
    COMMENT "Welcome to Austral OS"
    ICON "system-help"
    CATEGORIES "Utility;"
)
