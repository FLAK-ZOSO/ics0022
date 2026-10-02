#include <bundle.h>
#include <fiorino_log.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static int last_priority = -1;

void syslog(int priority, const char* format, ...) {
    (void)format;
    last_priority = priority;
}

enum { HEADER_LEN = FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE,
       OVERHEAD = HEADER_LEN + FIORINO_GCM_TAG_SIZE };

static void check_round_trip(size_t ciphertext_len) {
    unsigned char* ciphertext = malloc(ciphertext_len ? ciphertext_len : 1);
    assert(ciphertext != NULL);
    for (size_t i = 0; i < ciphertext_len; ++i)
        ciphertext[i] = (unsigned char)(i * 73 + 19);

    struct FiorinoBundle original = {
        .ciphertext_len = ciphertext_len,
        .ciphertext = ciphertext,
    };
    for (size_t i = 0; i < FIORINO_IV_SIZE; ++i)
        original.iv[i] = (unsigned char)(i + 1);
    for (size_t i = 0; i < FIORINO_PBKDF2_SALT_SIZE; ++i)
        original.salt[i] = (unsigned char)(255 - i);
    for (size_t i = 0; i < FIORINO_GCM_TAG_SIZE; ++i)
        original.tag[i] = (unsigned char)(i * 17);

    size_t serialized_len = 999;
    unsigned char* serialized = serialize_bundle(&original, &serialized_len);
    assert(serialized != NULL);
    assert(serialized_len == OVERHEAD + ciphertext_len);
    assert(memcmp(serialized, original.iv, FIORINO_IV_SIZE) == 0);
    assert(memcmp(serialized + FIORINO_IV_SIZE, original.salt,
                  FIORINO_PBKDF2_SALT_SIZE) == 0);
    assert(memcmp(serialized + HEADER_LEN, ciphertext, ciphertext_len) == 0);
    assert(memcmp(serialized + HEADER_LEN + ciphertext_len, original.tag,
                  FIORINO_GCM_TAG_SIZE) == 0);
    if (ciphertext_len != 0) {
        ciphertext[0] ^= 1;
        assert(serialized[HEADER_LEN] == 19);
        ciphertext[0] ^= 1;
    }

    struct FiorinoBundle* restored =
        deserialize_bundle(serialized, serialized_len);
    assert(restored != NULL);
    assert(restored->ciphertext == (unsigned char*)(restored + 1));
    assert(restored->ciphertext_len == ciphertext_len);
    assert(memcmp(restored->iv, original.iv, FIORINO_IV_SIZE) == 0);
    assert(memcmp(restored->salt, original.salt,
                  FIORINO_PBKDF2_SALT_SIZE) == 0);
    assert(memcmp(restored->ciphertext, ciphertext, ciphertext_len) == 0);
    assert(memcmp(restored->tag, original.tag, FIORINO_GCM_TAG_SIZE) == 0);

    /* Deserialization owns a copy; the input can be released first. */
    free(serialized);
    free(ciphertext);
    assert(restored->iv[0] == 1);
    assert(restored->salt[0] == 255);
    assert(restored->tag[0] == 0);
    if (ciphertext_len != 0)
        assert(restored->ciphertext[0] == 19);
    free(restored);
}

static void check_invalid_input(void) {
    unsigned char small[OVERHEAD + 1] = {0};
    assert(deserialize_bundle(NULL, OVERHEAD) == NULL);
    assert(last_priority == (LOG_USER | LOG_WARNING));
    for (size_t len = 0; len < OVERHEAD; ++len)
        assert(deserialize_bundle(small, len) == NULL);
    struct FiorinoBundle* empty = deserialize_bundle(small, OVERHEAD);
    assert(empty != NULL && empty->ciphertext_len == 0);
    free(empty);
    empty = deserialize_bundle(small, OVERHEAD + 1);
    assert(empty != NULL && empty->ciphertext_len == 1);
    free(empty);

    size_t out_len = 123;
    assert(serialize_bundle(NULL, &out_len) == NULL && out_len == 0);
    struct FiorinoBundle zero = {0};
    unsigned char* encoded_zero = serialize_bundle(&zero, &out_len);
    assert(encoded_zero != NULL && out_len == OVERHEAD);
    free(encoded_zero);
    struct FiorinoBundle invalid = { .ciphertext_len = 1 };
    out_len = 123;
    assert(serialize_bundle(&invalid, &out_len) == NULL && out_len == 0);
    invalid.ciphertext_len = FIORINO_MAX_FILE_SIZE + 1;
    invalid.ciphertext = small;
    out_len = 123;
    assert(serialize_bundle(&invalid, &out_len) == NULL && out_len == 0);
    assert(serialize_bundle(&invalid, NULL) == NULL);

    size_t too_long_len = OVERHEAD + FIORINO_MAX_FILE_SIZE + 1;
    unsigned char* too_long = malloc(too_long_len);
    assert(too_long != NULL);
    assert(deserialize_bundle(too_long, too_long_len) == NULL);
    free(too_long);
}

int main(void) {
    const size_t lengths[] = {0, 1, 15, 16, 17, 255, 4096,
                              FIORINO_MAX_FILE_SIZE};
    for (size_t i = 0; i < sizeof lengths / sizeof lengths[0]; ++i)
        check_round_trip(lengths[i]);
    check_invalid_input();
    return 0;
}
