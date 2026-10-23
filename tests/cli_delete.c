#define _GNU_SOURCE

#include <fiorino.h>

#include <assert.h>
#include <limits.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int run_cli(const char *program, const char *home, const char *vault,
                   const char *filename, const char *input)
{
    char cwd[PATH_MAX];
    assert(getcwd(cwd, sizeof cwd) != NULL);
    char library_path[PATH_MAX];
    int n = snprintf(library_path, sizeof library_path, "%s/lib", cwd);
    assert(n > 0 && (size_t)n < sizeof library_path);

    int terminal;
    pid_t child = forkpty(&terminal, NULL, NULL, NULL);
    assert(child >= 0);
    if (child == 0) {
        assert(setenv("HOME", home, 1) == 0);
        assert(setenv("LD_LIBRARY_PATH", library_path, 1) == 0);
        assert(chdir(home) == 0);
        if (vault != NULL)
            execl(program, program, "-v", vault, "-f", filename, (char *)NULL);
        else
            execl(program, program, "-f", filename, (char *)NULL);
        _exit(127);
    }

    size_t input_len = strlen(input);
    assert(write(terminal, input, input_len) == (ssize_t)input_len);
    int status;
    assert(waitpid(child, &status, 0) == child);
    close(terminal);
    if (!WIFEXITED(status))
        return 128 + WTERMSIG(status);
    return WEXITSTATUS(status);
}

static void make_path(char *out, size_t size, const char *home,
                      const char *suffix)
{
    int n = snprintf(out, size, "%s/%s", home, suffix);
    assert(n > 0 && (size_t)n < size);
}

int main(void)
{
    /* Create an encrypted fixture through the public encrypt CLI. */
    char home_template[] = "build/fiorino-delete-XXXXXX";
    assert(mkdtemp(home_template) != NULL);
    char home[PATH_MAX];
    assert(realpath(home_template, home) != NULL);
    assert(setenv("HOME", home, 1) == 0);

    char encrypt_path[PATH_MAX], delete_path[PATH_MAX];
    assert(realpath("./build/fiorino-encrypt", encrypt_path) != NULL);
    assert(realpath("./build/fiorino-delete", delete_path) != NULL);

    const char *vault_name = "delete-test";
    const char *filename = "sample.txt";
    char plaintext_path[PATH_MAX];
    make_path(plaintext_path, sizeof plaintext_path, home, filename);
    FILE *plaintext = fopen(plaintext_path, "wb");
    assert(plaintext != NULL);
    assert(fputs("keep the source file\n", plaintext) >= 0);
    assert(fclose(plaintext) == 0);

    assert(run_cli(encrypt_path, home, vault_name, filename,
                   "correct horse\ncorrect horse\n") == 0);

    char encrypted_path[PATH_MAX];
    int n = snprintf(encrypted_path, sizeof encrypted_path,
                     "%s/.fiorino/%s/%s.enc", home, vault_name, filename);
    assert(n > 0 && (size_t)n < sizeof encrypted_path);
    assert(access(encrypted_path, F_OK) == 0);

    /* Wrong password must preserve the encrypted file. */
    assert(run_cli(delete_path, home, vault_name, filename, "wrong password\n") != 0);
    assert(access(encrypted_path, F_OK) == 0);

    /* A multiply-linked encrypted file is refused and remains intact. */
    char linked_path[PATH_MAX];
    make_path(linked_path, sizeof linked_path, home, "linked-copy.enc");
    assert(link(encrypted_path, linked_path) == 0);
    struct Vault vault = {
        .user = geteuid(),
        .vault = vault_name,
        .passphrase = "correct horse",
    };
    assert(fiorino_delete(&vault, filename) == 2);
    assert(access(encrypted_path, F_OK) == 0);
    assert(access(linked_path, F_OK) == 0);
    assert(unlink(linked_path) == 0);

    /* A symlink entry is refused; its outside referent survives. */
    char outside_path[PATH_MAX], symlink_path[PATH_MAX];
    make_path(outside_path, sizeof outside_path, home, "outside.txt");
    make_path(symlink_path, sizeof symlink_path, home,
              ".fiorino/delete-test/symlink.enc");
    FILE *outside = fopen(outside_path, "wb");
    assert(outside != NULL);
    assert(fputs("must survive\n", outside) >= 0);
    assert(fclose(outside) == 0);
    assert(symlink(outside_path, symlink_path) == 0);
    assert(run_cli(delete_path, home, vault_name, "symlink",
                   "correct horse\n") != 0);
    assert(access(outside_path, F_OK) == 0);
    assert(access(symlink_path, F_OK) == 0);
    assert(unlink(symlink_path) == 0);

    /* Correct password deletes the encrypted entry but keeps the source. */
    assert(run_cli(delete_path, home, vault_name, filename,
                   "correct horse\n") == 0);
    assert(access(encrypted_path, F_OK) == -1);
    assert(access(plaintext_path, F_OK) == 0);
    assert(run_cli(delete_path, home, vault_name, filename,
                   "correct horse\n") != 0);

    /* Deleting the same entry again fails; remove all fixture files. */
    char auth_path[PATH_MAX], vault_path[PATH_MAX], root_path[PATH_MAX];
    n = snprintf(auth_path, sizeof auth_path, "%s/.fiorino/%s/.auth",
                 home, vault_name);
    assert(n > 0 && (size_t)n < sizeof auth_path);
    n = snprintf(vault_path, sizeof vault_path, "%s/.fiorino/%s",
                 home, vault_name);
    assert(n > 0 && (size_t)n < sizeof vault_path);
    make_path(root_path, sizeof root_path, home, ".fiorino");
    assert(unlink(auth_path) == 0);
    assert(rmdir(vault_path) == 0);
    assert(unlink(plaintext_path) == 0);
    assert(unlink(outside_path) == 0);
    assert(rmdir(root_path) == 0);
    assert(rmdir(home) == 0);

    puts("CLI delete tests passed");
    return 0;
}
