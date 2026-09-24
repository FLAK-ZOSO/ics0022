#include <fiorino.h>

/** \brief Decrypts a file from a vault
 * \param vault Identifier of the vault to decrypt from
 * \param files File to decrypt, by basename
 * \note The caller is responsible for the allocation of vault and filenames
 * \return Return code
 * \retval 0 success
 * \retval 1 failure
 */
int decryptFile(char* vault, char* file);
