// gcc -Wall -Wextra -g secure_account.c -o secure_account
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

void create_account() {
    char username[16];
    char filename[32];
    unsigned int age;

    printf("Username: ");
    scanf("%s", username);

    printf("Age: ");
    scanf("%u", &age);

    sprintf(filename, "users_%s.txt", username);

    int fd = open(filename,
        O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW,
        S_IRUSR | S_IWUSR
    );

    if (fd == -1) {
        if (errno == ELOOP) {
            // Failure specifically due to O_NOFOLLOW
            fprintf(stderr, "[FATAL] %s existed already, and as a symlink\n", filename);
            return;
        }
        fprintf(stderr, "[FATAL] Failure in opening %s\n", filename);
        return;
    }

    dprintf(fd, "Username: %s\n", username);
    dprintf(fd, "Age: %u\n", age);

    if (close(fd) == -1) {
        fprintf(stderr, "[FATAL] Could not close %s\n", filename);
        // close(2): "close() should not be retried after an error."
        return;
    }

    printf("Account successfully created: %s\n", filename);
}

int main() {
    int choice;

    printf("=== Account Manager ===\n");
    printf("1. Create account\n");
    printf("2. Exit\n");
    printf("Choice: ");

    scanf("%d", &choice);

    if (choice == 1) {
        create_account();
    } else {
        printf("Goodbye!\n");
    }

    return 0;
}
