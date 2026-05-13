#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <syslog.h>

#define PAM_SM_AUTH
#define SOCKET_TEMPLATE "/run/user/%d/horizon-keyring.socket"

static void scrub_memory(void *p, size_t len) {
    if (p == NULL) return;
    volatile unsigned char *pv = (volatile unsigned char *)p;
    while (len--) *pv++ = 0;
}

static int send_to_daemon(pam_handle_t *pamh, uid_t uid, const char *pass, const char *custom_path) {
    int sock = -1;
    struct sockaddr_un addr;
    
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        pam_syslog(pamh, LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return PAM_SERVICE_ERR;
    }

    // Set a short timeout for connection
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500ms
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    if (custom_path) {
        strncpy(addr.sun_path, custom_path, sizeof(addr.sun_path) - 1);
    } else {
        snprintf(addr.sun_path, sizeof(addr.sun_path), SOCKET_TEMPLATE, uid);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        // Not a hard error, daemon might not be running yet
        pam_syslog(pamh, LOG_DEBUG, "Could not connect to horizon-keyring socket at %s", addr.sun_path);
        close(sock);
        return PAM_SUCCESS; 
    }

    // Send the password
    // Protocol: [length(uint32_t)] + [password]
    uint32_t len = strlen(pass);
    if (write(sock, &len, sizeof(len)) != sizeof(len) || 
        write(sock, pass, len) != (ssize_t)len) {
        pam_syslog(pamh, LOG_ERR, "Failed to send password to daemon");
    } else {
        pam_syslog(pamh, LOG_INFO, "Successfully delivered password to horizon-keyring for UID %d", uid);
    }

    close(sock);
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *user = NULL;
    const char *pass = NULL;
    const char *custom_path = NULL;
    struct passwd *pwd;
    int retval;

    // Parse arguments
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "socket_path=", 12) == 0) {
            custom_path = argv[i] + 12;
        }
    }

    retval = pam_get_user(pamh, &user, NULL);
    if (retval != PAM_SUCCESS || user == NULL) {
        return PAM_USER_UNKNOWN;
    }

    pwd = getpwnam(user);
    if (pwd == NULL) {
        return PAM_USER_UNKNOWN;
    }

    // Capture the password from the PAM stack
    retval = pam_get_authtok(pamh, PAM_AUTHTOK, &pass, NULL);
    if (retval != PAM_SUCCESS || pass == NULL) {
        return PAM_SUCCESS;
    }

    // Attempt to deliver to daemon
    send_to_daemon(pamh, pwd->pw_uid, pass, custom_path);

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

#ifdef PAM_MODULE_ENTRY
PAM_MODULE_ENTRY("pam_horizon_keyring");
#endif
