#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int run_cli(const char *program, const char *home,
                   const char *filename, const char *input)
{
    char cwd[PATH_MAX];
    assert(getcwd(cwd, sizeof cwd) != NULL);
    char library_path[PATH_MAX];
    assert(snprintf(library_path, sizeof library_path, "%s/lib", cwd)
           < (int)sizeof library_path);

    int terminal;
    pid_t child = forkpty(&terminal, NULL, NULL, NULL);
    assert(child >= 0);

    if (child == 0) {
        assert(setenv("HOME", home, 1) == 0);
        assert(setenv("LD_LIBRARY_PATH", library_path, 1) == 0);
        assert(chdir(home) == 0);
        execl(program, program, "-f", filename, (char *)NULL);
        _exit(127);
    }

    size_t input_len = strlen(input);
    assert(write(terminal, input, input_len) == (ssize_t)input_len);

    int status;
    assert(waitpid(child, &status, 0) == child);
    close(terminal);
    if (!WIFEXITED(status)) {
        fprintf(stderr, "%s terminated by signal %d\n", program,
                WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return WEXITSTATUS(status);
}

int main(void)
{
    char home_template[] = "build/fiorino-cli-XXXXXX";
    char home_path[PATH_MAX];
    assert(mkdtemp(home_template) != NULL);
    assert(realpath(home_template, home_path) != NULL);
    char *home = home_path;

    char encrypt_path[PATH_MAX];
    char decrypt_path[PATH_MAX];
    assert(realpath("./build/fiorino-encrypt", encrypt_path) != NULL);
    assert(realpath("./build/fiorino-decrypt", decrypt_path) != NULL);

    const char *filename = "sample.txt";
    const char *plaintext = "CLI round-trip test\n";
    char input_path[PATH_MAX];
    assert(snprintf(input_path, sizeof input_path, "%s/%s", home, filename)
           < (int)sizeof input_path);
    FILE *input = fopen(input_path, "wb");
    assert(input != NULL);
    assert(fputs(plaintext, input) >= 0);
    assert(fclose(input) == 0);

    assert(run_cli(encrypt_path, home, filename, "correct horse\ncorrect horse\n") == 0);
    assert(unlink(input_path) == 0);

    assert(run_cli(decrypt_path, home, filename, "correct horse\n") == 0);

    input = fopen(input_path, "rb");
    assert(input != NULL);
    char recovered[sizeof "CLI round-trip test\n"];
    assert(fread(recovered, 1, sizeof recovered - 1, input) == sizeof recovered - 1);
    recovered[sizeof recovered - 1] = '\0';
    assert(fgetc(input) == EOF);
    assert(fclose(input) == 0);
    assert(strcmp(recovered, plaintext) == 0);

    puts("CLI round-trip test passed");
    return 0;
}
