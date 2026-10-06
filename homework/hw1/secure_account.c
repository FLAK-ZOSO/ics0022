// gcc -Wall -Wextra -g secure_account.c -o secure_account
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

void create_account() {
    char username[16];
    char filename[32];
    char age_buffer[5];
    unsigned long int age;

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
    for (size_t i = 0; i < 16; i++) {
        if (!isalnum(username[i])) {
            fprintf(stderr, "[FATAL] character %c in username is not alphanumeric\n", username[i]);
            return;
        }
    }

    printf("Age: ");
    if (!fgets(age_buffer, 5, stdin)) {
        fprintf(stderr, "[FATAL] could not read age\n");
        return;
    }
    char* sign = strchr(age_buffer, '-');
    if (sign) {
        fprintf(stderr, "[FATAL] age cannot be negative\n");
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

    if (snprintf(filename, 32, "users_%s.txt", username) <= 0) {
        fprintf(stderr, "[FATAL] something went wrong when formatting the filename\n");
    }

    int fd = open(filename,
        O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW,
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
    dprintf(fd, "Age: %lu\n", age);

    if (close(fd) == -1) {
        fprintf(stderr, "[FATAL] Could not close %s\n", filename);
        // close(2): "close() should not be retried after an error."
        return;
    }

    printf("Account successfully created: %s\n", filename);
}

int main() {
    long int choice;

    printf("=== Account Manager ===\n");
    printf("1. Create account\n");
    printf("2. Exit\n");
    printf("Choice: ");

    char age_buffer[3];

    if (!fgets(age_buffer, 3, stdin)) {
        fprintf(stderr, "[FATAL] could not read choice\n");
        return 1;
    }
    char* newline = strchr(age_buffer, '\n');
    if (newline) {
        *newline = '\0';
    } else {
        char c; // This is needed to consume the input in excess
        while((c = getchar()) != '\n' && c != EOF);
        fprintf(stderr, "[FATAL] choice too long, one digit is the maximum\n");
        return 1;
    }
    char* endptr; // Will store the first invalid character as per strtoul(3)

    errno = 0;
    choice = strtol(age_buffer, &endptr, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "[FATAL] this choice is out of range for an integer\n");
        return 1;
    } else if (endptr == age_buffer) {
        fprintf(stderr, "[FATAL] no digit was read from your provided choice\n");
        return 1;
    } else if (*endptr && *endptr != '\n') {
        // *endptr is neither end of string nor newline,
        // so we didn't convert the *whole* input
        fprintf(stderr, "[FATAL] only a prefix of your provided choice was valid digits\n");
        return 1;
    }
    if (choice <= 0 || choice > 2) {
        fprintf(stderr, "[FATAL] not a valid choice\n");
        return 1;
    }

    if (choice == 1) {
        create_account();
    } else {
        printf("Goodbye!\n");
    }

    return 0;
}
