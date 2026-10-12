#include <fiorino.h>
#include <fiorino_log.h>
#include <fiorino_cli.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (fiorino_cli_install_signal_handler(NULL) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING, "could not install", "signal handlers");
        fprintf(stderr, "Warning: could not install signal handlers");
        return EXIT_FAILURE;
    }
    int opt;
    const char* vault = "default";
    char* filename = NULL;
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

    char passphrase[100] = {0};
    fiorino_log_set_threshold(FIORINO_LOG_INFO);
    if (fiorino_read_password(passphrase, sizeof passphrase, 1) < 0) {
        fprintf(stderr, "Could not read password, aborting.\n");
        return EXIT_FAILURE;
    }
    if (fiorino_cli_raised()) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        int signo = fiorino_cli_signo();
        fiorino_log_message(FIORINO_LOG_INFO, "received signal %d, aborting", signo);
        fprintf(stderr, "Interrupted by %s, safely aborting.", strsignal(signo));
        return 128 + signo;
    }

    struct Vault vault_ = {
        geteuid(),
        vault,
        passphrase
    };

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        fprintf(stderr, "Opening the input stream failed, aborting.\n");
        return EXIT_FAILURE;
    }
    struct FileToEncrypt file_ = {
        file, filename
    };

    int encrypt_result = fiorino_encrypt_stream(&vault_, &file_);
    if (fiorino_cli_raised()) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        int signo = fiorino_cli_signo();
        fiorino_log_message(FIORINO_LOG_INFO, "received signal %d, aborting", signo);
        fprintf(stderr, "Interrupted by %s, safely aborting.", strsignal(signo));
        return 128 + signo;
    }
    if (encrypt_result != 0) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        if (encrypt_result == FIORINO_AUTHENTICATION_FAILED)
            fprintf(stderr, "Wrong vault password, aborting.\n");
        else
            fprintf(stderr, "Encryption failed, aborting.\n");
        if (fclose(file) == EOF) {
            fprintf(stderr, "Closing the input stream failed.\n");
        }
        return EXIT_FAILURE;
    }

    if (fclose(file) == EOF) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        fprintf(stderr, "Closing the input stream failed.\n");
        return EXIT_FAILURE;
    }

    fiorino_cleanse(passphrase, sizeof passphrase);
    printf("Your file has been encrypted successfully to the vault '%s' as '%s'\n",
            vault, filename);
    return EXIT_SUCCESS;
}
