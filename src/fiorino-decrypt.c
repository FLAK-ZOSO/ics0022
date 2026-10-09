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

    printf("vault:%s, filename:%s\n", vault, filename);
    return EXIT_SUCCESS;
}
