#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void create_account() {
    char username[16];
    char filename[32];
    unsigned int age;

    printf("Username: ");
    scanf("%s", username);

    printf("Age: ");
    scanf("%u", &age);

    sprintf(filename, "users_%s.txt", username);

    FILE *file = fopen(filename, "w");

    fprintf(file, "Username: %s\n", username);
    fprintf(file, "Age: %u\n", age);

    fclose(file);

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
