#pragma once
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

/** \brief Decrypts a file from a vault
 * \param vault Identifier of the vault to decrypt from
 * \param file File to decrypt, by basename
 * \note The caller is responsible for the allocation of vault and filename
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 */
int fiorino_decrypt(struct Vault* vault, char* file);

/** \brief Encrypts a file to a vault
 * \param vault The information of the vault to encrypt to
 * \param file File to encrypt, open in read binary mode
 * \note The caller is responsible for the allocation of the vault and members
 * \note The caller is responsible for closing the file
 * \note It overwrites pre-existing files, as a file manager should
 * \note On an OpenSSL failure, call ERR_get_error
 * \see https://docs.openssl.org/3.5/man3/ERR_get_error/
 * \todo Check all possible failures (fopen, file_size, fread, even malloc?)
 * \return Return code
 * \retval 0 success
 * \retval 1 other failure
 * \retval 2 OpenSSL failure
 */
int fiorino_encrypt(struct Vault* vault, struct FileToEncrypt* file);

/** \todo see functionality declared in the user interface */
