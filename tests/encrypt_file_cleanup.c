#include <fiorino.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int encrypt_file(struct Vault*, struct FileToEncrypt*);

int main(void) {
    char home[] = "/tmp/fiorino-cleanup-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);

    FILE* source = tmpfile();
    assert(source != NULL);
    const unsigned char message[] = {0, 1, 255, 0, 42};
    assert(fwrite(message, 1, sizeof message, source) == sizeof message);
    struct Vault vault = { .user = geteuid(), .vault = "test",
                           .passphrase = "secret" };
    struct FileToEncrypt file = { .file = source, .name = "sample" };
    assert(encrypt_file(&vault, &file) == 0);

    /* This fails after encryption and exercises the writer's cleanup path. */
    file.name = "missing/sample";
    assert(encrypt_file(&vault, &file) == 1);
    assert(fclose(source) == 0);

    const char* suffixes[] = {".enc", ".salt", ".iv"};
    char path[256];
    for (size_t i = 0; i < sizeof suffixes / sizeof suffixes[0]; ++i) {
        assert(snprintf(path, sizeof path, "%s/.fiorino/test/sample%s",
                        home, suffixes[i]) > 0);
        assert(unlink(path) == 0);
    }
    assert(snprintf(path, sizeof path, "%s/.fiorino/test", home) > 0);
    assert(rmdir(path) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino", home) > 0);
    assert(rmdir(path) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
