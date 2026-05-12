cmake_minimum_required(VERSION 3.10)
project(horizon-polkit-agent)

file(GLOB_RECURSE SOURCES 
    "*.cpp"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
)

add_executable(horizon-polkit-agent ${SOURCES})

target_link_libraries(horizon-polkit-agent
    PRIVATE
        horizon
)

# No necesitamos instalarlo como una app normal con icono si lo lanza la sesion,
# pero usar horizon_install_app es util para que termine en el binario correcto.
horizon_install_app(horizon-polkit-agent
    APP_ID "polkit-agent"
    NAME "Horizon Polkit Agent"
    COMMENT "Horizon Authentication Agent"
    ICON "dialog-password"
    VERSION "0.1.0"
    CATEGORIES "System;"
    EXTRA_DESKTOP "NoDisplay=true"
)
