#include <dirent.h> // DIR, https://en.wikibooks.org/wiki/C_programming/POSIX_reference/dirent.h

/** \brief Decrypts a list of files from a vault
 * \param vault Identifier of the vault to decrypt from
 * \param files Files to decrypt, by basename
 * \note The caller is responsible for the allocation of vault and filenames
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 */
int decrypt(char* vault, char** files);

/** \brief Encrypts a list of files to a vault
 * \param vault Identifier of the vault to encrypt to
 * \param files Files to encrypt, by path
 * \note The caller is responsible for the allocation of vault and filenames
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 */
int encrypt(char* vault, char** files);
