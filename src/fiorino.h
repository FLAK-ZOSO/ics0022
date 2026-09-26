#include <stdio.h>
#include <sys/types.h>

/* Maximum file size in bytes that the library will decrypt */
#define FIORINO_MAX_FILE_SIZE 10000000
#define FIORINO_VAULT_ROOT_DIR "~/.fiorino"
#define FIORINO_VAULT_DIR_PERM 0700
#define FIORINO_VAULT_PERM 0600

/** \brief Information needed to access a vault */
struct Vault {
    uid_t user; ///< claimed username
    char* vault; ///< selected vault
    char* passphrase; ///< attempted passphrase
};

/** \see https://stackoverflow.com/a/7988834/15888601 */
struct FileToEncrypt {
    FILE* file;
    char* name;
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
int fiorino_encrypt(struct Vault* vault, struct FileToEncrypt** files);

/** \todo see functionality declared in the user interface */
