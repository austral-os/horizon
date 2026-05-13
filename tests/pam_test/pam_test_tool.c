#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include <stdlib.h>

static struct pam_conv conv = {
    misc_conv,
    NULL
};

int main(int argc, char *argv[]) {
    pam_handle_t *pamh = NULL;
    int retval;
    const char *user = "testuser";

    if (argc > 1) user = argv[1];

    // Note: 'horizon-test' must be defined in /etc/pam.d/
    // or we can try to use a local config if pam allows it (usually not for security reasons)
    retval = pam_start("horizon-test", user, &conv, &pamh);

    if (retval == PAM_SUCCESS) {
        printf("Starting authentication for user: %s\n", user);
        retval = pam_authenticate(pamh, 0);
    }

    if (retval == PAM_SUCCESS)
        printf("Authentication SUCCESS\n");
    else
        printf("Authentication FAILED: %s\n", pam_strerror(pamh, retval));

    if (pam_end(pamh, retval) != PAM_SUCCESS) {
        pamh = NULL;
        printf("Failed to release authenticator\n");
        exit(1);
    }

    return (retval == PAM_SUCCESS) ? 0 : 1;
}
