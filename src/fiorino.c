#include <fiorino.h>
#include <encrypt.h>

#include <fcntl.h> // open
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
#define FIORINO_PBKDF2_SALT_SIZE 256
#define FIORINO_PBKDF2_ITERATIONS 600000

#define FIORINO_KEY_SIZE 256
#define FIORINO_IV_SIZE 128

/** \brief Decrypts a file from a vault
 * \param vault The information of the vault to decrypt from
 * \param files File to decrypt, by basename
 * \note The caller is responsible for the allocation of vault and filenames
 * \note On an OpenSSL failure, call ERR_get_error
 * \see https://docs.openssl.org/3.5/man3/ERR_get_error/
 * \return Return code
 * \retval 0 success
 * \retval 1 other failure
 * \retval 2 OpenSSL failure
 */
int decrypt_file(struct Vault*, char*);
/** \brief Returns the length of the file in bytes
 * \warning It leaves the cursor at the beginning of the file
 */
long long file_size(FILE*);

int fiorino_decrypt(struct Vault* vault, char** files) {
    
}

long long file_size(FILE* file) {
    /* We cannot really lose the race condition here
     * as this program is the only one touching the
     * vault directory and its files, really
     */
    fseek(file, 0, SEEK_END);
    long long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return fsize;
}

int decrypt_file(struct Vault* vault, char* file) {
    FILE* f = fopen(file, "rb");
    long long fsize = file_size(f);
    if (fsize > FIORINO_MAX_FILE_SIZE) return 1;
    unsigned char *const contents = malloc(fsize + 1);
    fread(contents, 1, fsize, f);
    /// \todo this would be a blunder, as padding is a thing
    unsigned char *const ciphertext = malloc(fsize + 1);
    unsigned char *const key = malloc(256);
    // https://crypto.stackexchange.com/questions/37629/pbkdf2-without-salt-on-16-digit-password
    // We really cannot do without a salt
    unsigned char *const salt = malloc(FIORINO_PBKDF2_SALT_SIZE);
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    int rc = RAND_priv_bytes(salt, FIORINO_PBKDF2_SALT_SIZE);
    if (rc == 0) return 2;
    // https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
    rc = PKCS5_PBKDF2_HMAC(
        vault->passphrase, -1,
        salt, FIORINO_PBKDF2_SALT_SIZE,
        FIORINO_PBKDF2_ITERATIONS, EVP_sha256(), // https://stackoverflow.com/a/63201320
        FIORINO_KEY_SIZE, key
    );
    if (rc == 0) return 2;
    unsigned char *const iv = malloc(FIORINO_IV_SIZE);
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    rc = RAND_priv_bytes(iv, FIORINO_IV_SIZE);
    if (rc == 0) return 2;
    rc = encrypt(contents, fsize + 1, key, iv, ciphertext);
    if (rc < 0) return 2;
}
