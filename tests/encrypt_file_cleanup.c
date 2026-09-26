#include <fiorino.h>
#include <bundle.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    assert(fiorino_encrypt(&vault, &file) == 0);
    assert(fclose(source) == 0);

    char path[256];
    assert(snprintf(path, sizeof path, "%s/.fiorino/test/sample.enc",
                    home) > 0);
    FILE* stored = fopen(path, "rb");
    assert(stored != NULL);
    unsigned char serialized[128];
    size_t serialized_len = fread(serialized, 1, sizeof serialized, stored);
    assert(fclose(stored) == 0);
    struct FiorinoBundle* bundle =
        deserialize_bundle(serialized, serialized_len);
    assert(bundle != NULL && bundle->ciphertext_len == sizeof message);
    free(bundle);
    assert(unlink(path) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino/test", home) > 0);
    assert(rmdir(path) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino", home) > 0);
    assert(rmdir(path) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
