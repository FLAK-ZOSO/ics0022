#define _POSIX_C_SOURCE 200809L

#include <fiorino.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    int previous_dir = open(".", O_RDONLY | O_DIRECTORY);
    assert(previous_dir >= 0);
    char home[] = "/tmp/fiorino-vault-symlink-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);
    assert(chdir(home) == 0);
    assert(mkdir(".fiorino", 0700) == 0);
    assert(mkdir("outside", 0700) == 0);
    assert(symlink("../outside", ".fiorino/evil") == 0);
    FILE *source = tmpfile();
    assert(source != NULL);
    static const unsigned char plaintext[] = "secret";
    assert(fwrite(plaintext, 1, sizeof plaintext, source) == sizeof plaintext);
    struct Vault vault = { geteuid(), "evil", "password" };
    struct FileToEncrypt input = { source, "payload" };
    /* This fails on vulnerable code, which follows the vault symlink. */
    assert(fiorino_encrypt_stream(&vault, &input) != 0);
    assert(fclose(source) == 0);
    assert(access("outside/.auth", F_OK) != 0);
    assert(access("outside/payload.enc", F_OK) != 0);
    assert(unlink(".fiorino/evil") == 0);
    assert(rmdir("outside") == 0);
    assert(rmdir(".fiorino") == 0);
    assert(fchdir(previous_dir) == 0);
    assert(close(previous_dir) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
