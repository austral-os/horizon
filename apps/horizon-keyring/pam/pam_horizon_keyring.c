#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#define SOCKET_PATH_TEMPLATE "/run/user/%d/horizon-keyring.socket"

static void send_password_to_daemon(pam_handle_t *pamh, int uid, const char *password) {
    int sock = 0;
    struct sockaddr_un serv_addr;
    char socket_path[256];

    snprintf(socket_path, sizeof(socket_path), SOCKET_PATH_TEMPLATE, uid);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return;
    }

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, socket_path, sizeof(serv_addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
        // Send password
        write(sock, password, strlen(password));
    }

    close(sock);
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *user;
    const char *password;
    struct passwd *pw;

    if (pam_get_user(pamh, &user, NULL) != PAM_SUCCESS) {
        return PAM_SUCCESS;
    }

    if (!(pw = getpwnam(user))) {
        return PAM_SUCCESS;
    }

    if (pam_get_item(pamh, PAM_AUTHTOK, (const void **)&password) != PAM_SUCCESS || !password) {
        return PAM_SUCCESS;
    }

    // Send the password to the daemon to unlock the keyring
    send_password_to_daemon(pamh, pw->pw_uid, password);

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
