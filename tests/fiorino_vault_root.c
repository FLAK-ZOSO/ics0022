#define _POSIX_C_SOURCE 200809L

#include <fiorino.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int list_vault(struct Vault* vault) {
    FILE* output = tmpfile();
    assert(output != NULL);
    int result = fiorino_list(vault, output);
    assert(fclose(output) == 0);
    return result;
}

int main(void) {
    int previous_dir = open(".", O_RDONLY | O_DIRECTORY);
    assert(previous_dir >= 0);
    char home[] = "/tmp/fiorino-vault-root-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);
    assert(chdir(home) == 0);

    struct Vault vault = {
        .user = geteuid(),
        .vault = "test",
        .passphrase = "password"
    };
    assert(list_vault(&vault) != 0);
    assert(access(".fiorino", F_OK) == -1);

    assert(mkdir(".fiorino", 0755) == 0);
    assert(list_vault(&vault) != 0);
    struct stat st;
    assert(lstat(".fiorino", &st) == 0);
    assert((st.st_mode & 07777) == 0755); /* Listing must not repair permissions. */
    assert(rmdir(".fiorino") == 0);

    assert(mkdir("outside", 0700) == 0);
    assert(symlink("outside", ".fiorino") == 0);
    assert(list_vault(&vault) != 0);
    assert(access("outside/test", F_OK) == -1);
    assert(unlink(".fiorino") == 0);
    assert(rmdir("outside") == 0);

    assert(fchdir(previous_dir) == 0);
    assert(close(previous_dir) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
