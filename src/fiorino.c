#include <fiorino.h>
#include <encrypt.h>

#include <fcntl.h> // open
#include <errno.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <wordexp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
#define FIORINO_PBKDF2_SALT_SIZE_BITS 256
#define FIORINO_PBKDF2_SALT_SIZE (FIORINO_PBKDF2_SALT_SIZE_BITS / 8)
#define FIORINO_PBKDF2_ITERATIONS 600000

#define FIORINO_KEY_SIZE_BITS 256
#define FIORINO_KEY_SIZE (FIORINO_KEY_SIZE_BITS / 8)
#define FIORINO_IV_SIZE_BITS 128
#define FIORINO_IV_SIZE (FIORINO_IV_SIZE_BITS / 8)

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

/** \brief Idempotently creates the vault root directory with correct permissions
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
        if (mkdir(vault_root_path.we_wordv[0], FIORINO_VAULT_DIR_PERM) == -1) {
            /// \todo log

        }
    } else if (FIORINO_VAULT_DIR_PERM != st.st_mode) {
        /// \todo log
        if (chmod(vault_root_path.we_wordv[0], FIORINO_VAULT_DIR_PERM) == -1) {
            /// \todo log
            return 1;
        }
    }
    wordfree(&vault_root_path);
    return 0;
}

/** \brief Idempotently creates a vault directory with correct permissions
 * \param vault The vault to be checked
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int vault_directory_exists(struct Vault* vault) {
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t vault_root_path;
    wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, 0);
    int vaultpath_alloc = strlen(vault_root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1;
    char* vaultpath = malloc(vaultpath_alloc);
    sprintf(vaultpath, "%s/%s",
            vault_root_path.we_wordv[0], vault->vault);
    wordfree(&vault_root_path);
    
    // https://stackoverflow.com/a/7430262/15888601
    struct stat st = {0};
    if (stat(vaultpath, &st) == -1) {
        /// \todo log
        if (mkdir(vaultpath, FIORINO_VAULT_DIR_PERM) == -1) {
            /// \todo log

        }
    } else if (FIORINO_VAULT_DIR_PERM != st.st_mode) {
        /// \todo log
        if (chmod(vaultpath, FIORINO_VAULT_DIR_PERM) == -1) {
            /// \todo log
            return 1;
        }
    }
    return 0;
}

/** \brief Idempotently creates a vault directory with correct permissions
 * \param vault The vault whose user is to be checked
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int vault_user_corresponds(struct Vault* vault) {
    /// \todo log
    return vault->user != geteuid();
}

int runtime_setup() {
    return vault_root_directory_exists();
}

int runtime_setup_vault(struct Vault* vault) {
    return vault_directory_exists(vault) || vault_user_corresponds(vault);
}

int encrypt_file(struct Vault* vault, struct FileToEncrypt* file) {
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    long long fsize = file_size(file->file);
    if (fsize > FIORINO_MAX_FILE_SIZE) return 1;
    unsigned char *const contents = malloc(fsize + 1);
    fread(contents, 1, fsize, file->file);
    unsigned char *const ciphertext = malloc(fsize + 16); // Padding
    unsigned char *const key = malloc(FIORINO_KEY_SIZE);
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
    int encryptrc = encrypt(contents, fsize, key, iv, ciphertext);
    if (encryptrc < 0) return 2;

    /// \todo check the goddamn filename and vault name
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t root_path;
    wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, 0);
    // https://stackoverflow.com/a/2153774/15888601
    int fullpath_alloc = strlen(root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1
        + strlen(file->name) + 4 + 1;
    char* fullpath = malloc(fullpath_alloc);
    char* fullpath_tmp = malloc(fullpath_alloc + 4);
    sprintf(fullpath, "%s/%s/%s.enc",
            root_path.we_wordv[0], vault->vault, file->name);
    sprintf(fullpath_tmp, "%s/%s/%s.enc.tmp",
            root_path.we_wordv[0], vault->vault, file->name);
    int saltpath_alloc = strlen(root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1
        + strlen(file->name) + 5 + 1;
    char* saltpath = malloc(saltpath_alloc);
    char* saltpath_tmp = malloc(saltpath_alloc + 4);
    sprintf(saltpath, "%s/%s/%s.salt",
            root_path.we_wordv[0], vault->vault, file->name);
    sprintf(saltpath_tmp, "%s/%s/%s.salt.tmp",
            root_path.we_wordv[0], vault->vault, file->name);
    int ivpath_alloc = strlen(root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1
        + strlen(file->name) + 3 + 1;
    char* ivpath = malloc(ivpath_alloc);
    char* ivpath_tmp = malloc(ivpath_alloc + 4);
    sprintf(ivpath, "%s/%s/%s.iv",
            root_path.we_wordv[0], vault->vault, file->name);
    sprintf(ivpath_tmp, "%s/%s/%s.iv.tmp",
            root_path.we_wordv[0], vault->vault, file->name);
    wordfree(&root_path);

    // https://stackoverflow.com/a/596337/15888601
    FILE* file_enc = fopen(fullpath_tmp, "wb");
    if (file_enc == NULL) {
        /// \todo log with errno
        return 1;
    }
    // Locking the files: https://stackoverflow.com/a/7573369/15888601
    // Unlocking the files: https://stackoverflow.com/a/33939497/15888601
    int lockrc = flock(fileno(file_enc), LOCK_SH);
    if (lockrc == -1) {
        /// \todo log with errno
    }
    if (chmod(fullpath_tmp, FIORINO_VAULT_PERM) == -1) {
        /// \todo log
    }

    FILE* file_salt = fopen(saltpath_tmp, "wb");
    if (file_salt == NULL) {
        /// \todo log with errno
        return 1;
    }
    lockrc = flock(fileno(file_salt), LOCK_SH);
    if (lockrc == -1) {
        /// \todo log with errno
    }
    if (chmod(saltpath_tmp, FIORINO_VAULT_PERM) == -1) {
        /// \todo log
    }
    FILE* file_iv = fopen(ivpath_tmp, "wb");
    if (file_iv == NULL) {
        /// \todo log with errno
        return 1;
    }
    lockrc = flock(fileno(file_iv), LOCK_SH);
    if (lockrc == -1) {
        /// \todo log with errno
    }
    if (chmod(ivpath_tmp, FIORINO_VAULT_PERM) == -1) {
        /// \todo log
    }

    if (!fwrite(ciphertext, 1, encryptrc, file_enc)) {
        /// \todo log with errno
    }
    lockrc = flock(fileno(file_enc), LOCK_UN);
    fclose(file_enc);

    if (!fwrite(iv, 1, FIORINO_IV_SIZE, file_iv)) {
        /// \todo log with errno
    }
    lockrc = flock(fileno(file_iv), LOCK_UN);
    fclose(file_iv);

    if (!fwrite(salt, 1, FIORINO_PBKDF2_SALT_SIZE, file_salt)) {
        /// \todo log with errno
    }
    lockrc = flock(fileno(file_salt), LOCK_UN);
    fclose(file_salt);

    /// \warning this approach cannot stay,
    /// as one could bork a file without being able to place the key,
    /// the only way is to rotate to *.bak temporary files
    if (rename(fullpath_tmp, fullpath) == -1) {
        /// \todo log with errno
        return 1;
    }
    if (rename(ivpath_tmp, ivpath) == -1) {
        /// \todo log with errno
        return 1;
    }
    if (rename(saltpath_tmp, saltpath) == -1) {
        /// \todo log with errno
        return 1;
    }

    free(fullpath_tmp);
    free(saltpath_tmp);
    free(ivpath_tmp);
    free(fullpath);
    free(saltpath);
    free(ivpath);

    return 0;
}
