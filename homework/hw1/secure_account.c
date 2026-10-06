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
    char age_buffer[5];
    unsigned int age;

    printf("Username: ");
    if (!fgets(username, 16, stdin)) {
        fprintf(stderr, "[FATAL] could not read username\n");
        return;
    }
    char* newline = strchr(username, '\n');
    if (newline) {
        *newline = '\0';
    } else {
        char c; // This is needed to consume the input in excess
        while((c = getchar()) != '\n' && c != EOF);
        fprintf(stderr, "[FATAL] username too long, 14 characters is the maximum\n");
        return;
    }

    printf("Age: ");
    if (!fgets(age_buffer, 5, stdin)) {
        fprintf(stderr, "[FATAL] could not read age\n");
        return;
    }
    newline = strchr(age_buffer, '\n');
    if (newline) {
        *newline = '\0';
    } else {
        char c; // This is needed to consume the input in excess
        while((c = getchar()) != '\n' && c != EOF);
        fprintf(stderr, "[FATAL] age too long, 3 digits is the maximum\n");
        return;
    }
    char* endptr; // Will store the first invalid character as per strtoul(3)

    errno = 0;
    age = strtoul(age_buffer, &endptr, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "[FATAL] this age is out of range for unsigned long\n");
        return;
    } else if (endptr == age_buffer) {
        fprintf(stderr, "[FATAL] no digit was read from your provided age\n");
        return;
    } else if (*endptr && *endptr != '\n') {
        // *endptr is neither end of string nor newline,
        // so we didn't convert the *whole* input
        fprintf(stderr, "[FATAL] only a prefix of your provided age was valid digits\n");
        return;
    }
    // This still allows for underflows as the function interprets "-" signs correctly
    // but we don't really care for now

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
    char c; // This is needed to consume the input in excess
    while((c = getchar()) != '\n' && c != EOF);

    if (choice == 1) {
        create_account();
    } else {
        printf("Goodbye!\n");
    }

    return 0;
}
