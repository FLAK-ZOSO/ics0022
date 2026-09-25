/** \file encrypt.h
 * \brief Contains helper functions from the OpenSSL Wiki
 * \see https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
 */

/** \brief Encrypts the provided plaintext
 * 
 */
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);

/** \brief Decrypts the provided plaintext
 * 
 */
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);
