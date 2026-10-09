#include <fiorino.h>
#include <bundle.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
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
    assert(fiorino_encrypt_stream(&vault, &file) == 0);
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

    source = tmpfile();
    assert(source != NULL);
    assert(fwrite(message, 1, sizeof message, source) == sizeof message);
    assert(fflush(source) == 0);
    file.file = source;
    file.name = "write-failure";
    struct rlimit original_limit;
    assert(getrlimit(RLIMIT_FSIZE, &original_limit) == 0);
    struct rlimit no_writes = original_limit;
    no_writes.rlim_cur = 0;
    void (*old_handler)(int) = signal(SIGXFSZ, SIG_IGN);
    assert(old_handler != SIG_ERR);
    assert(setrlimit(RLIMIT_FSIZE, &no_writes) == 0);
    assert(fiorino_encrypt_stream(&vault, &file) != 0);
    assert(setrlimit(RLIMIT_FSIZE, &original_limit) == 0);
    assert(signal(SIGXFSZ, old_handler) != SIG_ERR);
    assert(fclose(source) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino/test/write-failure.enc",
                    home) > 0);
    assert(access(path, F_OK) == -1);
    assert(snprintf(path, sizeof path, "%s/.fiorino/test/write-failure.enc.tmp",
                    home) > 0);
    assert(unlink(path) == 0);

    assert(snprintf(path, sizeof path, "%s/.fiorino/test/sample.enc",
                    home) > 0);
    assert(unlink(path) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino/test/.auth", home) > 0);
    assert(unlink(path) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino/test", home) > 0);
    assert(rmdir(path) == 0);
    assert(snprintf(path, sizeof path, "%s/.fiorino", home) > 0);
    assert(rmdir(path) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
