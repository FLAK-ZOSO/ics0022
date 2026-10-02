#include <bundle.h>
#include <fiorino_log.h>
#include <stdlib.h>
#include <string.h>

struct FiorinoBundle *deserialize_bundle(const unsigned char *contents, size_t contents_len) {
    const size_t header_len =
        FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE;
    const size_t overhead = header_len + FIORINO_GCM_TAG_SIZE;

    if (contents == NULL || contents_len < overhead) {
        fiorino_log_message(FIORINO_LOG_WARNING, "encrypted bundle is missing or truncated");
        return NULL;
    }

    const size_t ciphertext_len = contents_len - overhead;
    if (ciphertext_len > FIORINO_MAX_FILE_SIZE) {
        fiorino_log_message(FIORINO_LOG_WARNING, "encrypted bundle exceeds the size limit");
        return NULL;
    }

    /* The ciphertext lives immediately after the struct. */
    struct FiorinoBundle *bundle =
        malloc(sizeof *bundle + ciphertext_len);
    if (bundle == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "encrypted bundle");
        return NULL;
    }

    bundle->ciphertext = (unsigned char *)(bundle + 1);
    bundle->ciphertext_len = ciphertext_len;

    memcpy(bundle->iv, contents, FIORINO_IV_SIZE);
    memcpy(bundle->salt, contents + FIORINO_IV_SIZE, FIORINO_PBKDF2_SALT_SIZE);
    if (ciphertext_len != 0)
        memcpy(bundle->ciphertext, contents + header_len, ciphertext_len);
    memcpy(bundle->tag, contents + header_len + ciphertext_len, FIORINO_GCM_TAG_SIZE);

    return bundle;
}

unsigned char* serialize_bundle(const struct FiorinoBundle* bundle,
                                size_t* contents_len) {
    if (contents_len == NULL) {
        fiorino_log_message(FIORINO_LOG_WARNING, "missing serialized bundle length output");
        return NULL;
    }
    *contents_len = 0;
    if (bundle == NULL || bundle->ciphertext_len > FIORINO_MAX_FILE_SIZE ||
        (bundle->ciphertext_len != 0 && bundle->ciphertext == NULL)) {
        fiorino_log_message(FIORINO_LOG_WARNING, "invalid bundle for serialization");
        return NULL;
    }

    const size_t header_len = FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE;
    const size_t total_len = header_len + bundle->ciphertext_len +
                             FIORINO_GCM_TAG_SIZE;
    unsigned char* contents = malloc(total_len);
    if (contents == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "serialized bundle");
        return NULL;
    }

    memcpy(contents, bundle->iv, FIORINO_IV_SIZE);
    memcpy(contents + FIORINO_IV_SIZE, bundle->salt, FIORINO_PBKDF2_SALT_SIZE);
    if (bundle->ciphertext_len != 0)
        memcpy(contents + header_len, bundle->ciphertext, bundle->ciphertext_len);
    memcpy(contents + header_len + bundle->ciphertext_len, bundle->tag,
           FIORINO_GCM_TAG_SIZE);
    *contents_len = total_len;
    return contents;
}
