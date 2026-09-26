/** \file encrypt.h
 * \brief Contains helper functions from the OpenSSL Wiki
 * \see https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
 */
#pragma once
#include <bundle.h>

/** \brief Encrypts with AES-256-GCM.
 * \note ciphertext must have room for plaintext_len bytes; tag must have
 *       FIORINO_GCM_TAG_SIZE bytes.
 * \return Ciphertext length, or -1 on failure.
 */
int encrypt(const unsigned char *plaintext, int plaintext_len,
            const unsigned char *key, const unsigned char *iv,
            unsigned char *ciphertext,
            unsigned char tag[FIORINO_GCM_TAG_SIZE]);

/** \brief Authenticates and decrypts AES-256-GCM ciphertext.
 * \note ciphertext_len excludes the separate FIORINO_GCM_TAG_SIZE-byte tag.
 * \warning Discard all plaintext if this function returns -1; GCM may write
 *          plaintext before tag verification finishes.
 * \return Plaintext length, or -1 on failure (including invalid tag).
 */
int decrypt(const unsigned char *ciphertext, int ciphertext_len,
            const unsigned char tag[FIORINO_GCM_TAG_SIZE],
            const unsigned char *key, const unsigned char *iv,
            unsigned char *plaintext);
