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
    unsigned char tag[FIORINO_GCM_TAG_SIZE];
};

/** \brief Deserializes a bundled file into a FiorinoBundle structure
 * \warning The caller is responsible for deallocating the struct from the heap
 * \note The struct is allocated with a single `malloc` call and thus can be
 *       freed with a single `free` call
 * \param contents Serialized bytes in IV, salt, ciphertext, tag order
 * \param contents_len Number of bytes available at contents
 * \return Newly allocated bundle; its ciphertext is in the same allocation
 * \retval NULL when deserialization failed
 */
struct FiorinoBundle* deserialize_bundle(const unsigned char* contents, size_t contents_len);

/** \brief Serializes a bundle in IV, salt, ciphertext, tag order
 * \param bundle Bundle to serialize
 * \param contents_len Receives the allocated buffer length; set to zero on failure
 * \return Newly allocated bytes, freed by the caller with free(), or NULL
 */
unsigned char* serialize_bundle(const struct FiorinoBundle* bundle, size_t* contents_len);
