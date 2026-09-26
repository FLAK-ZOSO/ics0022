#include <encrypt.h>

#include <assert.h>
#include <string.h>

static void check(const unsigned char *message, int message_len) {
    unsigned char key[FIORINO_KEY_SIZE] = {0};
    unsigned char iv[FIORINO_IV_SIZE] = {0};
    unsigned char encrypted[64] = {0};
    unsigned char decrypted[64] = {0};

    int encrypted_len = encrypt((unsigned char *)message, message_len, key,
                                iv, encrypted);
    assert(encrypted_len == message_len + FIORINO_GCM_TAG_SIZE);
    assert(decrypt(encrypted, encrypted_len, key, iv, decrypted) == message_len);
    assert(memcmp(decrypted, message, (size_t)message_len) == 0);

    encrypted[encrypted_len - 1] ^= 1;
    assert(decrypt(encrypted, encrypted_len, key, iv, decrypted) == -1);
    encrypted[encrypted_len - 1] ^= 1;
    if (message_len > 0) {
        encrypted[0] ^= 1;
        assert(decrypt(encrypted, encrypted_len, key, iv, decrypted) == -1);
        encrypted[0] ^= 1;
    }
    iv[0] ^= 1;
    assert(decrypt(encrypted, encrypted_len, key, iv, decrypted) == -1);
    iv[0] ^= 1;
    key[0] ^= 1;
    assert(decrypt(encrypted, encrypted_len, key, iv, decrypted) == -1);
    assert(decrypt(encrypted, FIORINO_GCM_TAG_SIZE - 1, key, iv,
                   decrypted) == -1);
}

int main(void) {
    unsigned char message[] = {0, 1, 255, 0, 42};
    check(message, sizeof message);
    check(message, 0);
    return 0;
}
