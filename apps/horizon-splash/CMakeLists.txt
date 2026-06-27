project(horizon-splash)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/SplashApplication.cpp
    src/SplashWindow.cpp
    src/IpcServer.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    horizon
)

horizon_install_app(${PROJECT_NAME}
    APP_ID "horizon-splash"
    NAME "Splash"
    COMMENT "Horizon Splash Screen"
    CATEGORIES "System;"
    NO_DESKTOP
)
