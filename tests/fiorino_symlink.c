#include <fiorino.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    int previous_dir = open(".", O_RDONLY | O_DIRECTORY);
    assert(previous_dir >= 0);
    char home[] = "/tmp/fiorino-symlink-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);
    assert(chdir(home) == 0);

    const unsigned char secret[] = "decrypted vault contents";
    FILE* source = tmpfile();
    assert(source != NULL);
    assert(fwrite(secret, 1, sizeof secret, source) == sizeof secret);
    struct Vault vault = { .user = geteuid(), .vault = "test",
                           .passphrase = "password" };
    struct FileToEncrypt input = { .file = source, .name = "report" };
    assert(fiorino_encrypt(&vault, &input) == 0);
    assert(fclose(source) == 0);

    const unsigned char sentinel[] = "private file must not change";
    FILE* target = fopen("private.txt", "wb");
    assert(target != NULL);
    assert(fwrite(sentinel, 1, sizeof sentinel, target) == sizeof sentinel);
    assert(fclose(target) == 0);
    assert(symlink("private.txt", "report") == 0);

    /* A preexisting symlink must not redirect decrypted output. */
    int decrypt_result = fiorino_decrypt(&vault, "report");
    target = fopen("private.txt", "rb");
    assert(target != NULL);
    unsigned char actual[sizeof sentinel];
    size_t actual_len = fread(actual, 1, sizeof actual, target);
    assert(actual_len == sizeof sentinel &&
           memcmp(actual, sentinel, sizeof sentinel) == 0);
    assert(fclose(target) == 0);
    assert(decrypt_result != 0);

    assert(symlink("../../private.txt", ".fiorino/test/blocked.enc.tmp") == 0);
    source = tmpfile();
    assert(source != NULL);
    assert(fwrite(secret, 1, sizeof secret, source) == sizeof secret);
    struct FileToEncrypt blocked = { .file = source, .name = "blocked" };
    assert(fiorino_encrypt(&vault, &blocked) != 0);
    assert(fclose(source) == 0);
    target = fopen("private.txt", "rb");
    assert(target != NULL);
    assert(fread(actual, 1, sizeof actual, target) == sizeof actual);
    assert(memcmp(actual, sentinel, sizeof sentinel) == 0);
    assert(fclose(target) == 0);

    assert(unlink(".fiorino/test/blocked.enc.tmp") == 0);
    target = fopen(".fiorino/test/blocked.enc.tmp", "wb");
    assert(target != NULL);
    assert(fclose(target) == 0);
    source = tmpfile();
    assert(source != NULL);
    assert(fwrite(secret, 1, sizeof secret, source) == sizeof secret);
    assert(fiorino_encrypt(&vault, &(struct FileToEncrypt){
        .file = source, .name = "blocked" }) == 0);
    assert(fclose(source) == 0);
    assert(access(".fiorino/test/blocked.enc", F_OK) == 0);
    assert(access(".fiorino/test/blocked.enc.tmp", F_OK) != 0);

    assert(unlink("report") == 0);
    assert(unlink("private.txt") == 0);
    assert(unlink(".fiorino/test/blocked.enc") == 0);
    assert(unlink(".fiorino/test/report.enc") == 0);
    assert(rmdir(".fiorino/test") == 0);
    assert(rmdir(".fiorino") == 0);
    assert(fchdir(previous_dir) == 0);
    assert(close(previous_dir) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
