#include <bundle.h>
#include <fiorino.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_bytes(const char* path, const unsigned char* bytes,
                        size_t length) {
    FILE* file = fopen(path, "wb");
    assert(file != NULL);
    if (length != 0) assert(fwrite(bytes, 1, length, file) == length);
    assert(fclose(file) == 0);
}

static unsigned char* read_bytes(const char* path, size_t* length) {
    FILE* file = fopen(path, "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    assert(size >= 0);
    assert(fseek(file, 0, SEEK_SET) == 0);
    unsigned char* bytes = malloc(size ? (size_t)size : 1);
    assert(bytes != NULL);
    assert(fread(bytes, 1, (size_t)size, file) == (size_t)size);
    assert(fclose(file) == 0);
    *length = (size_t)size;
    return bytes;
}

static void assert_file_equals(const char* path, const unsigned char* expected,
                               size_t expected_len) {
    size_t actual_len;
    unsigned char* actual = read_bytes(path, &actual_len);
    assert(actual_len == expected_len);
    assert(memcmp(actual, expected, expected_len) == 0);
    free(actual);
}

static void encrypt_named(struct Vault* vault, const char* name,
                          const unsigned char* data, size_t length) {
    FILE* source = tmpfile();
    assert(source != NULL);
    if (length != 0) assert(fwrite(data, 1, length, source) == length);
    struct FileToEncrypt file = { .file = source, .name = (char*)name };
    assert(fiorino_encrypt(vault, &file) == 0);
    assert(fclose(source) == 0);
}

int main(void) {
    int previous_dir = open(".", O_RDONLY | O_DIRECTORY);
    assert(previous_dir >= 0);
    char home[] = "/tmp/fiorino-roundtrip-XXXXXX";
    assert(mkdtemp(home) != NULL);
    assert(setenv("HOME", home, 1) == 0);
    assert(chdir(home) == 0);

    struct Vault vault = { .user = geteuid(), .vault = "test",
                           .passphrase = "correct password" };
    const unsigned char message[] = {0, 1, 255, 0, 'A', '\n'};
    encrypt_named(&vault, "sample", message, sizeof message);
    assert(fiorino_decrypt(&vault, "sample") == 0);
    assert_file_equals("sample", message, sizeof message);

    const unsigned char sentinel[] = "keep this file";
    write_bytes("sample", sentinel, sizeof sentinel);
    struct Vault wrong = vault;
    wrong.passphrase = "wrong password";
    assert(fiorino_decrypt(&wrong, "sample") != 0);
    assert_file_equals("sample", sentinel, sizeof sentinel);

    const char* bundle_path = ".fiorino/test/sample.enc";
    size_t bundle_len;
    unsigned char* original = read_bytes(bundle_path, &bundle_len);
    const size_t header_len = FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE;
    assert(bundle_len > header_len + FIORINO_GCM_TAG_SIZE);
    const size_t tamper_offsets[] = {
        0, FIORINO_IV_SIZE, header_len, bundle_len - 1
    };
    for (size_t i = 0; i < sizeof tamper_offsets / sizeof tamper_offsets[0]; ++i) {
        unsigned char* changed = malloc(bundle_len);
        assert(changed != NULL);
        memcpy(changed, original, bundle_len);
        changed[tamper_offsets[i]] ^= 1;
        write_bytes(bundle_path, changed, bundle_len);
        free(changed);
        assert(fiorino_decrypt(&vault, "sample") != 0);
        assert_file_equals("sample", sentinel, sizeof sentinel);
    }

    write_bytes(bundle_path, original,
                header_len + FIORINO_GCM_TAG_SIZE - 1);
    assert(fiorino_decrypt(&vault, "sample") != 0);
    assert_file_equals("sample", sentinel, sizeof sentinel);
    write_bytes(bundle_path, original, bundle_len);
    free(original);

    assert(fiorino_decrypt(&vault, "missing") != 0);
    assert(access("missing", F_OK) != 0);

    encrypt_named(&vault, "empty", NULL, 0);
    assert(fiorino_decrypt(&vault, "empty") == 0);
    assert_file_equals("empty", NULL, 0);

    const unsigned char outside_sentinel[] = "this file is outside the vault";
    write_bytes("outside.enc", outside_sentinel, sizeof outside_sentinel);
    FILE* traversal_source = tmpfile();
    assert(traversal_source != NULL);
    assert(fwrite(message, 1, sizeof message, traversal_source) == sizeof message);
    struct FileToEncrypt traversal = {
        .file = traversal_source, .name = "../../outside"
    };
    assert(fiorino_encrypt(&vault, &traversal) != 0);
    assert(fclose(traversal_source) == 0);
    assert_file_equals("outside.enc", outside_sentinel, sizeof outside_sentinel);

    assert(unlink("sample") == 0);
    assert(unlink("empty") == 0);
    assert(unlink("outside.enc") == 0);
    assert(unlink(".fiorino/test/sample.enc") == 0);
    assert(unlink(".fiorino/test/empty.enc") == 0);
    assert(rmdir(".fiorino/test") == 0);
    assert(rmdir(".fiorino") == 0);
    assert(fchdir(previous_dir) == 0);
    assert(close(previous_dir) == 0);
    assert(rmdir(home) == 0);
    return 0;
}
