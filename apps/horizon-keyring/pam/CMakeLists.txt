cmake_minimum_required(VERSION 3.10)
project(libpam-horizon-keyring)

add_library(pam_horizon_keyring SHARED pam_horizon_keyring.c)
target_link_libraries(pam_horizon_keyring PRIVATE pam)
set_target_properties(pam_horizon_keyring PROPERTIES PREFIX "")

install(TARGETS pam_horizon_keyring DESTINATION /lib/security COMPONENT keyring)

# Install PAM config for pam-auth-update
install(FILES horizon-keyring.pam-config
    DESTINATION share/pam-configs
    RENAME horizon-keyring
    COMPONENT keyring
)
