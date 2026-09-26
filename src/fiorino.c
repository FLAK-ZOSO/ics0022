#include <fiorino.h>
#include <encrypt.h>

#include <fcntl.h> // open
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
#define FIORINO_PBKDF2_SALT_SIZE 256
#define FIORINO_PBKDF2_ITERATIONS 600000

#define FIORINO_KEY_SIZE 256
#define FIORINO_IV_SIZE 128

/** \brief Encrypts a file from a vault
 * \param vault The information of the vault to encrypt to
 * \param files File to encrypt, open in read binary mode
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
int encrypt_file(struct Vault*, struct FileToEncrypt*);
/** \brief Returns the length of the file in bytes
 * \warning It leaves the cursor at the beginning of the file
 */
long long file_size(FILE*);
/** \brief Verifies that the system and runtime conditions are met
 * \note Must be strictly idemponent (e.g. don't delete .fiorino/* by mistake)
 * \return State
 * \retval 0 success
 * \retval 1 failure
 */
int runtime_setup();

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

/**
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int vault_root_directory_exists() {
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t vault_root_path;
    wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, 0);
    // https://stackoverflow.com/a/7430262/15888601
    struct stat st = {0};
    if (stat(vault_root_path.we_wordv[0], &st) == -1) {
        /// \todo log
        if (mkdir(vault_root_path.we_wordv[0], FIORINO_VAULT_PERM) == -1) {
            /// \todo log

        }
    } else if (FIORINO_VAULT_PERM != st.st_mode) {
        /// \todo log
        chmod(vault_root_path.we_wordv[0], FIORINO_VAULT_PERM);
    }
    wordfree(&vault_root_path);
    return 0;
}

int runtime_setup() {
    return vault_root_directory_exists();
}

int encrypt_file(struct Vault* vault, struct FileToEncrypt* file) {
    if (runtime_setup()) return 1;
    long long fsize = file_size(file->file);
    if (fsize > FIORINO_MAX_FILE_SIZE) return 1;
    unsigned char *const contents = malloc(fsize + 1);
    fread(contents, 1, fsize, file->file);
    unsigned char *const ciphertext = malloc(fsize + 16); // Padding
    unsigned char *const key = malloc(FIORINO_KEY_SIZE / 8);
    // https://crypto.stackexchange.com/questions/37629/pbkdf2-without-salt-on-16-digit-password
    // We really cannot do without a salt
    unsigned char *const salt = malloc(FIORINO_PBKDF2_SALT_SIZE / 8);
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    int rc = RAND_priv_bytes(salt, FIORINO_PBKDF2_SALT_SIZE / 8);
    if (rc == 0) return 2;
    // https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
    rc = PKCS5_PBKDF2_HMAC(
        vault->passphrase, -1,
        salt, FIORINO_PBKDF2_SALT_SIZE / 8,
        FIORINO_PBKDF2_ITERATIONS, EVP_sha256(), // https://stackoverflow.com/a/63201320
        FIORINO_KEY_SIZE / 8, key
    );
    if (rc == 0) return 2;
    unsigned char *const iv = malloc(FIORINO_IV_SIZE / 8);
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    rc = RAND_priv_bytes(iv, FIORINO_IV_SIZE / 8);
    if (rc == 0) return 2;
    rc = encrypt(contents, fsize, key, iv, ciphertext);
    if (rc < 0) return 2;

    /// \todo check the goddamn filename and vault name
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t root_path;
    wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, 0);
    // https://stackoverflow.com/a/2153774/15888601
    char* fullpath = malloc(
        strlen(root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1
        + strlen(file->name) + 4 + 1
    );
    sprintf(fullpath, "%s/%s/%s.enc",
            root_path.we_wordv[0], vault->vault, file->name);
    char* saltpath = malloc(
        strlen(root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1
        + strlen(file->name) + 5 + 1
    );
    sprintf(saltpath, "%s/%s/%s.salt",
            root_path.we_wordv[0], vault->vault, file->name);
    char* ivpath = malloc(
        strlen(root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1
        + strlen(file->name) + 3 + 1
    );
    sprintf(ivpath, "%s/%s/%s.iv",
            root_path.we_wordv[0], vault->vault, file->name);
    wordfree(&root_path);
    // https://stackoverflow.com/a/596337/15888601
    int file_enc = open(fullpath, O_CREAT|O_WRONLY, FIORINO_VAULT_PERM);
    int file_salt = open(saltpath, O_CREAT|O_WRONLY, FIORINO_VAULT_PERM);
    int file_iv = open(ivpath, O_CREAT|O_WRONLY, FIORINO_VAULT_PERM);
    free(fullpath);
    free(saltpath);
    free(ivpath);
    /// \todo finish this goddamn function

}
