#include <fiorino-decrypt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
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

    char passphrase[100] = {0};
    if (fiorino_read_password(passphrase, sizeof passphrase, 0) < 0) {
        fprintf(stderr, "Could not read password, aborting.\n");
        return EXIT_FAILURE;
    }

    struct Vault vault_ = {
        geteuid(),
        vault,
        passphrase
    };

    if (fiorino_decrypt(&vault_, filename)) {
        fiorino_cleanse(passphrase, sizeof passphrase);
        fprintf(stderr, "Decryption failed, aborting.\n");
        return EXIT_FAILURE;
    }

    fiorino_cleanse(passphrase, sizeof passphrase);
    puts("Your file has been decrypted successfully");
    return EXIT_SUCCESS;
}
