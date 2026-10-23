#include <fiorino.h>
#include <fiorino_log.h>
#include <fiorino_cli.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

int main(int argc, char** argv) {
    if (fiorino_cli_install_signal_handler(NULL) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING, "could not install", "signal handlers");
        fprintf(stderr, "Warning: could not install signal handlers");
        return EXIT_FAILURE;
    }
    int opt;
    const char* vault = "default";
    while ((opt = getopt(argc, argv, "v:")) != -1) {
        switch (opt) {
            case 'v': {
                vault = optarg;
                break;
            }
            default: {
                fprintf(stderr, "Usage: %s [-v <vault>]\n", argv[0]);
                return EXIT_FAILURE;
            }
        }
    }

    char passphrase[100] = {0};
    fiorino_log_set_threshold(FIORINO_LOG_INFO);
    int password_result = fiorino_read_password(passphrase, sizeof passphrase, 1);
    if (fiorino_cli_raised()) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        int signo = fiorino_cli_signo();
        fiorino_log_message(FIORINO_LOG_INFO, "received signal %d, aborting", signo);
        fprintf(stderr, "Interrupted by %s, safely aborting.", strsignal(signo));
        return 128 + signo;
    }
    if (password_result < 0) {
        fprintf(stderr, "Could not read password, aborting.\n");
        return EXIT_FAILURE;
    }

    struct Vault vault_ = {
        geteuid(),
        vault,
        passphrase
    };
    int list_rc = fiorino_list(&vault_, stdout);
    switch (list_rc) {
        case FIORINO_AUTHENTICATION_FAILED: {
            fprintf(stderr, "Authentication to the vault '%s' failed", vault);
        }
        case 1: {
            fprintf(stderr, "Failed to list the contents of the vault '%s' failed", vault);
        }
    }

    fiorino_cleanse(passphrase, sizeof passphrase);
    if (!list_rc)
        fprintf(stderr, "Contents of the vault '%s' successfully listed\n", vault);
    int output_failed = 0;
    if (fflush(stdout) == EOF || ferror(stdout))
        output_failed = 1;
    if (fflush(stderr) == EOF || ferror(stderr))
        output_failed = 1;

    if (fiorino_cli_raised())
        return 128 + fiorino_cli_signo();
    if (output_failed || list_rc)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
