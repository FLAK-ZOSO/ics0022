#include <stdio.h>

/* Maximum file size in bytes that the library will decrypt */
#define FIORINO_MAX_FILE_SIZE 10'000'000

struct Vault {
    char* user;
    char* vault;
    char* passphrase;
};

/** \brief Decrypts a list of files from a vault
 * \param vault Identifier of the vault to decrypt from
 * \param files Files to decrypt, by basename
 * \note The caller is responsible for the allocation of vault and filenames
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 */
int fiorino_decrypt(struct Vault* vault, char** files);

/** \brief Encrypts a list of files to a vault
 * \param vault Identifier of the vault to encrypt to
 * \param files Files to encrypt
 * \note The caller is responsible for the allocation of vault and filenames
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 */
int fiorino_encrypt(struct Vault* vault, FILE** files);

/** \todo see functionality declared in the user interface */
