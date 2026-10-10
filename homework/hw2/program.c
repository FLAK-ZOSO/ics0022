#include <stdio.h>
#include <string.h>

void show_status(char *username) {
    printf("Logged in as: %s\n", username);
}

int login(char *username, char *input) {
    char password[12];
    int authenticated = 0;

    strcpy(password, input);

    if (strcmp(password, "secure123") == 0) {
        authenticated = 1;
    }

    if (authenticated) {
        printf("ACCESS GRANTED\n");
        show_status(username);
    } else {
        printf("ACCESS DENIED\n");
    }

    return authenticated;
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Usage: %s <username> <password>\n", argv[0]);
        return 1;
    }

    login(argv[1], argv[2]);

    return 0;
}