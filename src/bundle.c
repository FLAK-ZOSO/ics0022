#include <bundle.h>
#include <stdlib.h>
#include <string.h>

struct FiorinoBundle *deserialize_bundle(const unsigned char *contents, size_t contents_len) {
    const size_t header_len =
        FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE;
    const size_t overhead = header_len + FIORINO_GCM_TAG_SIZE;

    if (contents == NULL || contents_len < overhead)
        return NULL;

    const size_t ciphertext_len = contents_len - overhead;
    if (ciphertext_len > FIORINO_MAX_FILE_SIZE)
        return NULL;

    /* The ciphertext lives immediately after the struct. */
    struct FiorinoBundle *bundle =
        malloc(sizeof *bundle + ciphertext_len);
    if (bundle == NULL)
        return NULL;

    bundle->ciphertext = (unsigned char *)(bundle + 1);
    bundle->ciphertext_len = ciphertext_len;

    memcpy(bundle->iv, contents, FIORINO_IV_SIZE);
    memcpy(bundle->salt, contents + FIORINO_IV_SIZE,
           FIORINO_PBKDF2_SALT_SIZE);
    memcpy(bundle->ciphertext, contents + header_len, ciphertext_len);
    memcpy(bundle->tag, contents + header_len + ciphertext_len,
           FIORINO_GCM_TAG_SIZE);

    return bundle;
}
