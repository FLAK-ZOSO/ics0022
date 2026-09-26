/** \file encrypt.h
 * \brief Contains helper functions from the OpenSSL Wiki
 * \see https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
 */

/** \brief Encrypts the provided plaintext
 * \warning AES-256-CBC is what it is (padding oracle, unauthenticated ciphertext),
 *          thus we may want to use AES-256-GCM
 * \see https://csrc.nist.gov/pubs/sp/800/38/a/final
 * \see https://cheatsheetseries.owasp.org/cheatsheets/Cryptographic_Storage_Cheat_Sheet.html
 * \see https://wiki.openssl.org/index.php/EVP#Working_with_Algorithms_and_Modes
 * \todo on error exit there has to be cleanup, currently there isn't
 */
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);

/** \brief Decrypts the provided plaintext
 * \warning AES-256-CBC is what it is (padding oracle, unauthenticated ciphertext),
 *          thus we may want to use AES-256-GCM
 * \see https://csrc.nist.gov/pubs/sp/800/38/a/final
 * \see https://cheatsheetseries.owasp.org/cheatsheets/Cryptographic_Storage_Cheat_Sheet.html
 * \see https://wiki.openssl.org/index.php/EVP#Working_with_Algorithms_and_Modes
 * \todo on error exit there has to be cleanup, currently there isn't
 */
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);
