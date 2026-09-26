#include <fiorino.h>
#include <bundle.h>
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
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, WRDE_NOCMD) != 0)
        return 1;
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
            wordfree(&vault_root_path);
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
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, WRDE_NOCMD) != 0)
        return 1;
    int vaultpath_alloc = strlen(vault_root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1;
    char* vaultpath = malloc(vaultpath_alloc);
    if (vaultpath == NULL) {
        wordfree(&vault_root_path);
        return 1;
    }
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
            free(vaultpath);
            return 1;
        }
    }
    free(vaultpath);
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

static char* output_path(const char* root, const char* vault,
                         const char* name, const char* suffix) {
    int length = snprintf(NULL, 0, "%s/%s/%s%s", root, vault, name, suffix);
    if (length < 0) return NULL;

    char* path = malloc((size_t)length + 1);
    if (path != NULL)
        snprintf(path, (size_t)length + 1, "%s/%s/%s%s",
                 root, vault, name, suffix);
    return path;
}

static FILE* open_output_file(const char* path) {
    FILE* output = fopen(path, "wb");
    if (output == NULL) return NULL;

    int lockrc = flock(fileno(output), LOCK_SH);
    if (lockrc == -1) {
        /// \todo log with errno
    }
    if (chmod(path, FIORINO_VAULT_PERM) == -1) {
        /// \todo log
    }
    return output;
}

static FILE* open_input_file(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return NULL;

    struct stat st;
    const off_t min_size = FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE +
                           FIORINO_GCM_TAG_SIZE;
    if (fstat(fd, &st) != 0) {
        int error = errno;
        close(fd);
        errno = error;
        return NULL;
    }
    if (!S_ISREG(st.st_mode) || st.st_uid != geteuid() ||
        st.st_size < min_size ||
        st.st_size > min_size + FIORINO_MAX_FILE_SIZE) {
        close(fd);
        errno = EINVAL;
        return NULL;
    }

    FILE* input = fdopen(fd, "rb");
    if (input == NULL) {
        int error = errno;
        close(fd);
        errno = error;
    }
    return input;
}

static void write_output_file(FILE* output, const unsigned char* data,
                              size_t length) {
    if (!fwrite(data, 1, length, output)) {
        /// \todo log with errno
    }
    flock(fileno(output), LOCK_UN);
    fclose(output);
}

int fiorino_encrypt(struct Vault* vault, struct FileToEncrypt* file) {
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    long long fsize = file_size(file->file);
    if (fsize > FIORINO_MAX_FILE_SIZE) return 1;
    unsigned char *const contents = malloc(fsize + 1);
    fread(contents, 1, fsize, file->file);
    unsigned char *const ciphertext = malloc(fsize + 1);
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
    unsigned char tag[FIORINO_GCM_TAG_SIZE];
    int encryptrc = encrypt(contents, fsize, key, iv, ciphertext, tag);
    if (encryptrc < 0) return 2;

    /// \todo check the goddamn filename and vault name
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t root_path;
    wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, 0);
    char* fullpath = output_path(root_path.we_wordv[0], vault->vault, file->name, ".enc");
    char* fullpath_tmp = output_path(root_path.we_wordv[0], vault->vault, file->name, ".enc.tmp");
    wordfree(&root_path);

    struct FiorinoBundle bundle = {
        .ciphertext_len = (size_t)encryptrc,
        .ciphertext = ciphertext,
    };
    memcpy(bundle.iv, iv, sizeof bundle.iv);
    memcpy(bundle.salt, salt, sizeof bundle.salt);
    memcpy(bundle.tag, tag, sizeof bundle.tag);
    size_t serialized_len = 0;
    unsigned char* serialized = serialize_bundle(&bundle, &serialized_len);
    if (serialized == NULL) return 1;


    FILE* file_enc = open_output_file(fullpath_tmp);
    if (file_enc == NULL) {
        /// \todo log with errno
        free(serialized);
        return 1;
    }
    // Locking the files: https://stackoverflow.com/a/7573369/15888601
    // Unlocking the files: https://stackoverflow.com/a/33939497/15888601

    write_output_file(file_enc, serialized, serialized_len);
    free(serialized);

    if (rename(fullpath_tmp, fullpath) == -1) {
        /// \todo log with errno
        return 1;
    }

    free(fullpath_tmp);
    free(fullpath);

    return 0;
}

int fiorino_decrypt(struct Vault* vault, char* file) {
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    /// \todo do the due checks on this input argument
    // if (runtime_setup_file(file)) return 1;

    wordexp_t root_path;
    wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, 0);
    /// \todo Be careful with that "file", who knows what buffer overflows are coming
    char* fullpath = output_path(root_path.we_wordv[0], vault->vault, file, ".enc");
    FILE* file_enc = open_input_file(fullpath);
    long long fsize = file_size(file_enc); // Very needed to check the file is not a crafted bomb payload
    wordfree(&root_path);

    /// \todo finish this function, just began
}
