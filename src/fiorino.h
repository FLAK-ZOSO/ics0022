#pragma once
#include <stdio.h>
#include <sys/types.h>

/* Maximum file size in bytes that the library will decrypt */
#define FIORINO_MAX_FILE_SIZE 10000000
#define FIORINO_VAULT_ROOT_DIR "~/.fiorino"
#define FIORINO_VAULT_DIR_PERM 0700
#define FIORINO_VAULT_PERM 0600
/** Return code used when the supplied password does not match the vault. */
#define FIORINO_AUTHENTICATION_FAILED 3

/** \brief Information needed to access a vault */
struct Vault {
    uid_t user; ///< claimed username
    const char* vault; ///< selected vault
    char* passphrase; ///< attempted passphrase
};

/** \brief Open input stream and the desired name for its encrypted output.
 * \note name is a single filename, not a path. It is independent of the
 *       input stream's pathname; the input may have no pathname at all.
 */
struct FileToEncrypt {
    FILE* file; ///< Open stream supplying the plaintext bytes.
    char* name; ///< Desired basename of the encrypted file in the vault.
};

/** \brief Decrypts a file from a vault
 * \param vault Identifier of the vault to decrypt from
 * \param file File to decrypt, by basename
 * \note The caller is responsible for the allocation of vault and filename
 * \note It will, as expected by a file manager, overwrite the current omonymous file in cwd
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 * \retval FIORINO_AUTHENTICATION_FAILED vault authentication failed
 */
int fiorino_decrypt(struct Vault* vault, const char* file);

/** \brief Encrypts a file to a vault
 * \param vault The information of the vault to encrypt to
 * \param file Open plaintext stream and an independent encrypted output name
 * \note The caller is responsible for the allocation of the vault and members
 * \note The caller is responsible for closing the file
 * \note It overwrites pre-existing files, as a file manager should
 * \note The supplied stream (file->file) must be a seekable regular file
 *       whose ownership belongs to the caller, but whose locking and unlocking
 *       is handled by the library.
 * \note On an OpenSSL failure, call ERR_get_error
 * \see https://docs.openssl.org/3.5/man3/ERR_get_error/
 * \todo Check all possible failures (fopen, file_size, fread, even malloc?)
 * \return Return code
 * \retval 0 success
 * \retval 1 other failure
 * \retval 2 OpenSSL failure
 * \retval FIORINO_AUTHENTICATION_FAILED vault authentication failed
 */
int fiorino_encrypt_stream(struct Vault* vault, struct FileToEncrypt* file);

/** \brief Read a passphrase without echoing it on the terminal.
 * \param buffer Destination buffer, including space for the terminating NUL
 * \param buffer_size Size of buffer in bytes
 * \param verify If nonzero, prompt twice and require matching entries
 * \return 0 on success, negative value on failure
 * \note This uses OpenSSL's UI helper. During the prompt, OpenSSL may
 *       temporarily replace process signal dispositions and restore them
 *       afterwards. A signal received during that interval can therefore
 *       appear only as a password-read failure to the CLI.
 */
int fiorino_read_password(char *buffer, size_t buffer_size, int verify);

void fiorino_cleanse(void* buffer, size_t size);

/** \todo see functionality declared in the user interface */
