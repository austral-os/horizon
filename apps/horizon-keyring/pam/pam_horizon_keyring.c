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
#include <sys/stat.h>

#define PAM_SM_AUTH
#define SOCKET_TEMPLATE "/tmp/horizon-keyring-%d.socket"

static void scrub_memory(void *p, size_t len) {
    if (p == NULL) return;
    volatile unsigned char *pv = (volatile unsigned char *)p;
    while (len--) *pv++ = 0;
}

static int write_to_temp_file(pam_handle_t *pamh, uid_t uid, const char *pass) {
    char path[128];
    snprintf(path, sizeof(path), "/tmp/horizon-pass-%u", uid);

    pam_syslog(pamh, LOG_INFO, "Attempting to store password in %s", path);

    FILE *f = fopen(path, "w");
    if (!f) {
        pam_syslog(pamh, LOG_ERR, "Failed to create temp password file: %s", strerror(errno));
        return PAM_SERVICE_ERR;
    }

    // Secure permissions: only the user can read it
    if (fchmod(fileno(f), 0600) < 0) {
        pam_syslog(pamh, LOG_ERR, "Failed to set permissions on temp file: %s", strerror(errno));
        fclose(f);
        unlink(path);
        return PAM_SERVICE_ERR;
    }

    // Change ownership to the target user
    if (fchown(fileno(f), uid, -1) < 0) {
        pam_syslog(pamh, LOG_ERR, "Failed to set owner on temp file: %s", strerror(errno));
        fclose(f);
        unlink(path);
        return PAM_SERVICE_ERR;
    }

    if (fputs(pass, f) == EOF) {
        pam_syslog(pamh, LOG_ERR, "Failed to write password to temp file");
        fclose(f);
        unlink(path);
        return PAM_SERVICE_ERR;
    }

    fclose(f);
    pam_syslog(pamh, LOG_INFO, "Securely stored password for daemon at %s", path);
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char *user = NULL;
    const char *pass = NULL;
    struct passwd *pwd;
    int retval;

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

    // Attempt to deliver via temp file
    write_to_temp_file(pamh, pwd->pw_uid, pass);

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

#ifdef PAM_MODULE_ENTRY
PAM_MODULE_ENTRY("pam_horizon_keyring");
#endif
