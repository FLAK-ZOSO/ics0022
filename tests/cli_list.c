#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int run_cli(const char* program, const char* home,
                   char* const args[], const char* input,
                   char* output, size_t output_size) {
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
        execv(program, args);
        _exit(127);
    }

    size_t input_len = strlen(input);
    size_t sent = 0;
    while (sent < input_len) {
        ssize_t nwrite = write(terminal, input + sent, input_len - sent);
        if (nwrite < 0 && errno == EINTR) continue;
        assert(nwrite > 0);
        sent += (size_t)nwrite;
    }

    size_t used = 0;
    char chunk[512];
    for (;;) {
        ssize_t nread = read(terminal, chunk, sizeof chunk);
        if (nread < 0 && errno == EINTR) continue;
        if (nread < 0 && errno == EIO) break; /* PTY closed by child. */
        if (nread <= 0) break;
        size_t copy = (size_t)nread;
        if (copy > output_size - used - 1) copy = output_size - used - 1;
        if (copy != 0) {
            memcpy(output + used, chunk, copy);
            used += copy;
        }
    }
    output[used] = '\0';
    close(terminal);

    int status;
    assert(waitpid(child, &status, 0) == child);
    if (!WIFEXITED(status)) return 128 + WTERMSIG(status);
    return WEXITSTATUS(status);
}

static void make_path(char* out, size_t size, const char* home,
                      const char* relative) {
    int n = snprintf(out, size, "%s/%s", home, relative);
    assert(n > 0 && (size_t)n < size);
}

int main(void) {
    /* Create an isolated HOME and a file/vault fixture for CLI list cases. */
    char home_template[] = "build/fiorino-list-XXXXXX";
    assert(mkdtemp(home_template) != NULL);
    char home[PATH_MAX];
    assert(realpath(home_template, home) != NULL);

    char encrypt_path[PATH_MAX], list_path[PATH_MAX];
    assert(realpath("./build/fiorino-encrypt", encrypt_path) != NULL);
    assert(realpath("./build/fiorino-list", list_path) != NULL);

    const char* vault = "list-test";
    const char* filename = "sample.txt";
    char input_path[PATH_MAX];
    make_path(input_path, sizeof input_path, home, filename);
    FILE* input = fopen(input_path, "wb");
    assert(input != NULL);
    assert(fputs("listing test data\n", input) >= 0);
    assert(fclose(input) == 0);

    char output[8192];
    char* encrypt_args[] = {
        encrypt_path, "-v", (char*)vault, "-f", (char*)filename, NULL
    };
    assert(run_cli(encrypt_path, home, encrypt_args,
                   "correct horse\ncorrect horse\n",
                   output, sizeof output) == 0);

    /* A valid password lists the logical filename without the .enc suffix. */
    char* list_args[] = { list_path, "-v", (char*)vault, NULL };
    assert(run_cli(list_path, home, list_args, "correct horse\n",
                   output, sizeof output) == 0);
    assert(strstr(output, "sample.txt\r\n") != NULL ||
           strstr(output, "sample.txt\n") != NULL);
    assert(strstr(output, "sample.txt.enc") == NULL);

    /* A wrong password is rejected with an authentication diagnostic. */
    assert(run_cli(list_path, home, list_args, "wrong password\n",
                   output, sizeof output) != 0);
    assert(strstr(output, "Authentication") != NULL);

    /* Listing an absent vault fails and does not create that vault. */
    char* missing_args[] = { list_path, "-v", "missing-list-vault", NULL };
    assert(run_cli(list_path, home, missing_args, "correct horse\n",
                   output, sizeof output) != 0);
    char missing_path[PATH_MAX];
    make_path(missing_path, sizeof missing_path, home,
              ".fiorino/missing-list-vault");
    assert(access(missing_path, F_OK) == -1);

    /* Unknown command-line options are rejected. */
    char* bad_option_args[] = { list_path, "-x", NULL };
    assert(run_cli(list_path, home, bad_option_args, "", output,
                   sizeof output) != 0);

    char auth_path[PATH_MAX], encrypted_path[PATH_MAX];
    make_path(auth_path, sizeof auth_path, home, ".fiorino/list-test/.auth");
    make_path(encrypted_path, sizeof encrypted_path,
              home, ".fiorino/list-test/sample.txt.enc");
    assert(unlink(auth_path) == 0);
    assert(unlink(encrypted_path) == 0);
    char vault_path[PATH_MAX], root_path[PATH_MAX];
    make_path(vault_path, sizeof vault_path, home, ".fiorino/list-test");
    make_path(root_path, sizeof root_path, home, ".fiorino");
    assert(rmdir(vault_path) == 0);
    assert(unlink(input_path) == 0);
    assert(rmdir(root_path) == 0);
    assert(rmdir(home) == 0);

    puts("CLI list tests passed");
    return 0;
}
