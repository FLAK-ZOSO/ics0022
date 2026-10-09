#include <fiorino.h>
#include <fiorino_log.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
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

    if (fiorino_encrypt_stream(&vault_, &file_)) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        fprintf(stderr, "Decryption failed, aborting.\n");
        return EXIT_FAILURE;
    }

    if (fclose(file) == EOF) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        fprintf(stderr, "Closing the input stream failed.\n");
        return EXIT_FAILURE;
    }

    fiorino_cleanse(passphrase, sizeof passphrase);
    puts("Your file has been decrypted successfully");
    return EXIT_SUCCESS;
}
