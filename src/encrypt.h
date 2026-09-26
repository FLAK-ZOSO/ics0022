/** \file encrypt.h
 * \brief Contains helper functions from the OpenSSL Wiki
 * \see https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
 */
#pragma once
#include <bundle.h>

/** \brief Encrypts with AES-256-GCM.
 * \note ciphertext must have plaintext_len + FIORINO_GCM_TAG_SIZE bytes.
 * \return Ciphertext plus tag length, or -1 on failure.
 */
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);

/** \brief Authenticates and decrypts AES-256-GCM ciphertext.
 * \note ciphertext_len includes the trailing FIORINO_GCM_TAG_SIZE bytes.
 * \warning Discard all plaintext if this function returns -1; GCM may write
 *          plaintext before tag verification finishes.
 * \return Plaintext length, or -1 on failure (including invalid tag).
 */
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);
