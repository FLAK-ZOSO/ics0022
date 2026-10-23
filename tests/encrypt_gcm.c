#include <encrypt.h>
#include <fiorino_log.h>

#include <assert.h>
#include <string.h>
#include <syslog.h>

static int last_priority = -1;

void syslog(int priority, const char* format, ...) {
    (void)format;
    last_priority = priority;
}

/* Round trip, then reject modified tag/ciphertext/IV/key and invalid length. */
static void test_gcm_authentication_and_round_trip(const unsigned char *message,
                                                  int message_len) {
    unsigned char key[FIORINO_KEY_SIZE] = {0};
    unsigned char iv[FIORINO_IV_SIZE] = {0};
    unsigned char encrypted[64] = {0};
    unsigned char tag[FIORINO_GCM_TAG_SIZE] = {0};
    unsigned char decrypted[64] = {0};

    int encrypted_len = fiorino_encrypt_gcm(message, message_len, key, iv, encrypted, tag);
    assert(encrypted_len == message_len);
    assert(fiorino_decrypt_gcm(encrypted, encrypted_len, tag, key, iv, decrypted) == message_len);
    assert(memcmp(decrypted, message, (size_t)message_len) == 0);

    tag[FIORINO_GCM_TAG_SIZE - 1] ^= 1;
    assert(fiorino_decrypt_gcm(encrypted, encrypted_len, tag, key, iv, decrypted) == -1);
    assert(last_priority == (LOG_USER | LOG_WARNING));
    tag[FIORINO_GCM_TAG_SIZE - 1] ^= 1;
    if (message_len > 0) {
        encrypted[0] ^= 1;
        assert(fiorino_decrypt_gcm(encrypted, encrypted_len, tag, key, iv, decrypted) == -1);
        encrypted[0] ^= 1;
    }
    iv[0] ^= 1;
    assert(fiorino_decrypt_gcm(encrypted, encrypted_len, tag, key, iv, decrypted) == -1);
    iv[0] ^= 1;
    key[0] ^= 1;
    assert(fiorino_decrypt_gcm(encrypted, encrypted_len, tag, key, iv, decrypted) == -1);
    assert(fiorino_decrypt_gcm(encrypted, -1, tag, key, iv, decrypted) == -1);
}

int main(void) {
    /* Exercise both non-empty binary plaintext and the empty-message boundary. */
    unsigned char message[] = {0, 1, 255, 0, 42};
    test_gcm_authentication_and_round_trip(message, sizeof message);
    test_gcm_authentication_and_round_trip(message, 0);
    return 0;
}
