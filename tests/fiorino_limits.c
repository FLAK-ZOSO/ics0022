#include <fiorino.h>

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    /* Reject vault and output names whose complete paths exceed PATH_MAX. */
    int previous_dir = open(".", O_RDONLY | O_DIRECTORY);
    assert(previous_dir >= 0);
    char home[] = "/tmp/fiorino-limits-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);
    assert(chdir(home) == 0);

    FILE* input = tmpfile();
    assert(input != NULL);
    assert(fputc('x', input) == 'x');
    struct Vault vault = { .user = geteuid(), .vault = "test",
                           .passphrase = "password" };

    /* One byte beyond the full vault path's PATH_MAX limit. */
    size_t root_len = strlen(home) + strlen("/.fiorino");
    size_t vault_len = (size_t)PATH_MAX - root_len - 1;
    char* long_vault_name = malloc(vault_len + 1);
    assert(long_vault_name != NULL);
    memset(long_vault_name, 'v', vault_len);
    long_vault_name[vault_len] = '\0';
    struct Vault long_vault = vault;
    long_vault.vault = long_vault_name;
    struct FileToEncrypt file = { .file = input, .name = "sample" };
    assert(fiorino_encrypt_stream(&long_vault, &file) != 0);
    assert(fiorino_decrypt(&long_vault, "sample") != 0);
    free(long_vault_name);

    /* A filename that cannot fit in the full encrypted output path. */
    char long_output_name[PATH_MAX + 1];
    memset(long_output_name, 'n', PATH_MAX);
    long_output_name[PATH_MAX] = '\0';
    file.name = long_output_name;
    assert(fiorino_encrypt_stream(&vault, &file) != 0);
    assert(fiorino_decrypt(&vault, long_output_name) != 0);

    /* Reject a sparse input whose declared size exceeds the library limit. */
    assert(ftruncate(fileno(input), FIORINO_MAX_FILE_SIZE + 1) == 0);
    file.name = "oversized";
    assert(fiorino_encrypt_stream(&vault, &file) != 0);
    assert(access(".fiorino/test/oversized.enc", F_OK) == -1);

    assert(fclose(input) == 0);
    assert(unlink(".fiorino/test/.auth") == 0);
    assert(rmdir(".fiorino/test") == 0);
    assert(rmdir(".fiorino") == 0);
    assert(fchdir(previous_dir) == 0);
    assert(close(previous_dir) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
