/* Intentionally vulnerable local teaching program.
   Expected observations are for the supplied unoptimized Linux build.
   Writing outside name[] is undefined behavior in C, even when the bytes
   happen to belong to another member of the same struct. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session {
    char name[8];
    char role[8];
};

_Static_assert(offsetof(struct Session, role) == 8,
               "This classroom demonstration expects role at offset 8");
_Static_assert(sizeof(struct Session) == 16,
               "This classroom demonstration expects a 16-byte struct");

static void vulnerable_copy(struct Session *s, const char *input)
{
    /* BUG: strcpy does not know that the destination member has 8 bytes. */
    strcpy(s->name, input);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s NAME\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Classroom limit: keep examples within the 16-byte enclosing struct.
       This is NOT a correct bound for the 8-byte name member. */
    if (strlen(argv[1]) > 15) {
        fputs("Use at most 15 characters for this demonstration.\n", stderr);
        return EXIT_FAILURE;
    }

    struct Session session = {{0}, "USER"};
    printf("Role before copy: %s\n", session.role);
    vulnerable_copy(&session, argv[1]);
    printf("Role after copy: %s\n", session.role);

    if (strcmp(session.role, "ADMIN") == 0) {
        puts("ADMIN access granted: training-only secret = BLUE-MOON");
    } else {
        puts("ADMIN access denied.");
    }
    return EXIT_SUCCESS;
}
