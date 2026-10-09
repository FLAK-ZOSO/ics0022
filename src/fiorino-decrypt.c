#include <fiorino-decrypt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int opt;
    char* vault = "default";
    char* filename;
    while ((opt = getopt(argc, argv, "vf")) != -1) {
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
                exit(EXIT_FAILURE);
            }
        }
    }
    printf("vault:%s, filename:%s", vault, filename);
    exit(EXIT_SUCCESS);
}
