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
    const char* filename = NULL;
    while ((opt = getopt(argc, argv, "v:f:")) != -1) {
        switch (opt) {
            case 'v': {
                vault = optarg;
                break;
            }
            case 'f': {
                filename = optarg;
                break;
            }
            default: {
                fprintf(stderr, "Usage: %s -f <file> [-v <vault>]\n", argv[0]);
                return EXIT_FAILURE;
            }
        }
    }
    if (filename == NULL) {
        fprintf(stderr, "Filename must be specified, use -f to do so.\n");
        return EXIT_FAILURE;
    }

    if (fprintf(stderr,
                "Deleting the encrypted copy of '%s' from vault '%s'; the original source file is not affected.\n",
                filename, vault) < 0 || fflush(stderr) == EOF)
        return EXIT_FAILURE;

    char passphrase[100] = {0};
    fiorino_log_set_threshold(FIORINO_LOG_INFO);
    int password_result = fiorino_read_password(passphrase, sizeof passphrase, 0);
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

    int delete_result = fiorino_delete(&vault_, filename);
    if (delete_result != 0) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        if (delete_result == FIORINO_AUTHENTICATION_FAILED)
            fprintf(stderr, "Wrong vault password, aborting.\n");
        else
            fprintf(stderr, "Deletion failed, aborting.\n");
        return EXIT_FAILURE;
    }

    fiorino_cleanse(passphrase, sizeof passphrase);
    puts("Your file has been deleted successfully");
    int output_failed = 0;
    if (fflush(stdout) == EOF || ferror(stdout))
        output_failed = 1;
    if (fflush(stderr) == EOF || ferror(stderr))
        output_failed = 1;

    if (fiorino_cli_raised())
        return 128 + fiorino_cli_signo();
    if (output_failed)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
