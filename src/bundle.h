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

struct FiorinoBundle {
    unsigned char iv[FIORINO_IV_SIZE];
    unsigned char salt[FIORINO_PBKDF2_SALT_SIZE];
    unsigned char* ciphertext;
    size_t ciphertext_len;
};

struct FiorinoBundle deserialize_bundle(unsigned char*);

unsigned char* serialize_bundle(struct FiorinoBundle*);
