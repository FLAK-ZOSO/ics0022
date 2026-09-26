#pragma once
#include <fiorino.h>

// https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
#define FIORINO_PBKDF2_SALT_SIZE_BITS 256
#define FIORINO_PBKDF2_SALT_SIZE (FIORINO_PBKDF2_SALT_SIZE_BITS / 8)
#define FIORINO_PBKDF2_ITERATIONS 600000

#define FIORINO_KEY_SIZE_BITS 256
#define FIORINO_KEY_SIZE (FIORINO_KEY_SIZE_BITS / 8)
#define FIORINO_IV_SIZE_BITS 128
#define FIORINO_IV_SIZE (FIORINO_IV_SIZE_BITS / 8)
#define FIORINO_GCM_TAG_SIZE_BITS 128
#define FIORINO_GCM_TAG_SIZE (FIORINO_GCM_TAG_SIZE_BITS / 8)

/** \brief The core contents of a serialized Fiorino bundle */
struct FiorinoBundle {
    unsigned char iv[FIORINO_IV_SIZE];
    unsigned char salt[FIORINO_PBKDF2_SALT_SIZE];
    size_t ciphertext_len;
    unsigned char* ciphertext;
    unsigned char* tag[FIORINO_GCM_TAG_SIZE];
};

/** \brief Deserializes a bundled file into a FiorinoBundle structure
 * \warning The caller is responsible for deallocating the struct from the heap
 * \note The struct is allocated with a single `malloc` call and thus can be
 *       freed with a single `free` call
 * \param contents Allocated memory for the bundle
 * \param contents_len Size of the allocated memory
 * \return FiorinoBundle containing the deserialized data
 * \retval NULL when deserialization failed
 */
struct FiorinoBundle* deserialize_bundle(unsigned char* contents, size_t contents_len);

unsigned char* serialize_bundle(struct FiorinoBundle*);
