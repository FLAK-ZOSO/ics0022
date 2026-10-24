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

static void create_auth_record(struct Vault* vault) {
    FILE* source = tmpfile();
    assert(source != NULL);
    assert(fputs("auth record test\n", source) >= 0);
    struct FileToEncrypt input = { .file = source, .name = "sample" };
    assert(fiorino_encrypt_stream(vault, &input) == 0);
    assert(fclose(source) == 0);
}

int main(void) {
    int previous_dir = open(".", O_RDONLY | O_DIRECTORY);
    assert(previous_dir >= 0);
    char home[] = "/tmp/fiorino-auth-record-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);
    assert(chdir(home) == 0);

    struct Vault vault = {
        .user = geteuid(),
        .vault = "test",
        .passphrase = "password",
    };
    const char* auth_path = ".fiorino/test/.auth";

    /* A regular auth record owned by this user is accepted. */
    create_auth_record(&vault);
    assert(list_vault(&vault) == 0);

    /* O_NOFOLLOW rejects a symlink at the auth-record path. */
    assert(unlink(auth_path) == 0);
    FILE* target = fopen("auth-target", "wb");
    assert(target != NULL);
    assert(fputs("not an auth record", target) >= 0);
    assert(fclose(target) == 0);
    assert(symlink("../../auth-target", auth_path) == 0);
    assert(list_vault(&vault) != 0);
    assert(unlink(auth_path) == 0);

    /* Non-regular records, including FIFOs, fail without blocking on open. */
    assert(mkfifo(auth_path, 0600) == 0);
    assert(list_vault(&vault) != 0);
    assert(unlink(auth_path) == 0);

    /* Create a valid record again, then reject it if its owner differs. */
    create_auth_record(&vault);
    uid_t other_uid = geteuid() == (uid_t)0 ? (uid_t)1 : (uid_t)0;
    if (chown(auth_path, other_uid, (gid_t)-1) == 0)
        assert(list_vault(&vault) != 0);

    assert(unlink(auth_path) == 0);
    assert(unlink(".fiorino/test/sample.enc") == 0);
    assert(unlink("auth-target") == 0);
    assert(rmdir(".fiorino/test") == 0);
    assert(rmdir(".fiorino") == 0);
    assert(fchdir(previous_dir) == 0);
    assert(close(previous_dir) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
