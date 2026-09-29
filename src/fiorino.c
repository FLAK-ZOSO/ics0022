#include <fiorino.h>
#include <bundle.h>
#include <encrypt.h>

#include <fcntl.h> // open
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/file.h>
#include <string.h>
#include <wordexp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#define FIORINO_MAX_ENCRYPTED_SIZE (FIORINO_MAX_FILE_SIZE \
                                    + FIORINO_IV_SIZE \
                                    + FIORINO_GCM_TAG_SIZE \
                                    + FIORINO_PBKDF2_SALT_SIZE )

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
    if (strstr(vault->vault, "/") || strcmp(vault->vault, "..") == 0 ||
        strcmp(vault->vault, ".") == 0) {
        /// \todo log
        return 1;
    }
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t vault_root_path;
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, WRDE_NOCMD) != 0)
        return 1;
    int vaultpath_alloc = strlen(vault_root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1;
    if (vaultpath_alloc > PATH_MAX) {
        /// \todo log
        return 1;
    }
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
    return vault_directory_exists(vault)
        || vault_user_corresponds(vault);
}

/** \see https://stackoverflow.com/a/4553076/15888601 */
int is_regular_file(const char *path) {
    struct stat path_stat;
    /// \see https://stackoverflow.com/a/2635948/15888601
    if (lstat(path, &path_stat) == -1) {
        /// \todo log
        return -1;
    }
    return S_ISREG(path_stat.st_mode);
}

/** \brief Validates the encrypted output name
 * \param name The requested filename inside the vault
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int runtime_setup_output_name(const char* name) {
    if (strstr(name, "/") || strcmp(name, "..") == 0 ||
        strcmp(name, ".") == 0) {
        /// \todo log
        return 1;
    }
    return 0;
}

/** \brief Validates the open plaintext file and obtains its size
 * \note Leaves the stream positioned at the beginning for encryption
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int runtime_setup_plaintext_file(FILE* input, long long* size) {
    if (input == NULL || size == NULL) return 1;
    int fd = fileno(input);
    struct stat st;
    if (fd < 0) {
        /// \todo log "could not retrieve open file descriptor"
        return 1;
    } else if (fseek(input, 0, SEEK_SET) != 0) {
        /// \todo log "could not set cursor to the beginning of the file"
        return 1;
    } else if (fstat(fd, &st) == -1) {
        /// \todo log "could not retrieve file statistics"
        return 1;
    } else if (!S_ISREG(st.st_mode)) {
        /// \todo log "not a regular file"
        return 1;
    } else if (st.st_size < 0) {
        /// \todo log "could not retrieve file size"
        return 1;
    } else if (st.st_size > FIORINO_MAX_FILE_SIZE) {
        /// \todo log "file too big for encryption by Fiorino"
        return 1;
    }
    *size = st.st_size;
    return 0;
}

static char* output_path(const char* root, const char* vault,
                         const char* name, const char* suffix);

/** \brief Verifies that the file path is fine to decrypt
 * \warning The function allocates memory for the full path,
 *          the caller is responsible for deallocation
 * \return Full path to the encrypted file
 * \retval NULL failure
 */
char* runtime_setup_file(const struct Vault* vault, const char* file) {
    /** \warning we must ensure not to fall in a race condition with an attacker
     *           replacing that path with a dangerous symlink
     * \note Check for path traversal and other attacks
     * \note Reject .enc file extensions, as that is an internal implementation detail
     */
    if (strstr(file, "/") || strcmp(file, "..") == 0 ||
        strcmp(file, ".") == 0) {
        /// \todo log
        return NULL;
    }

    wordexp_t root_path;
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, WRDE_NOCMD) != 0) {
        /** \todo log */
        return NULL;
    }
    char* fullpath = output_path(root_path.we_wordv[0], vault->vault, file, ".enc");
    wordfree(&root_path);
    if (fullpath == NULL) {
        /// \todo log
        return NULL;
    }
    int regular = is_regular_file(fullpath);
    if (regular < 0) {
        /// \todo log
        free(fullpath);
        return NULL;
    } else if (regular == 0) {
        /// \todo log
        free(fullpath);
        return NULL;
    }
    return fullpath;
}

static char* output_path(const char* root, const char* vault,
                         const char* name, const char* suffix) {
    int length = snprintf(NULL, 0, "%s/%s/%s%s", root, vault, name, suffix);
    if (length < 0) {
        /// \todo log
        return NULL;
    } else if (length + 4 > PATH_MAX) { // Could never fit ".tmp"
        /// \todo log
        return NULL;
    }

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
    if (runtime_setup_output_name(file->name)) {
        /// \todo log
        return 1;
    }

    long long fsize;
    if (runtime_setup_plaintext_file(file->file, &fsize)) return 1;
    size_t contents_len = (size_t)fsize + 1;
    unsigned char* contents = malloc(contents_len);
    unsigned char* ciphertext = malloc(contents_len);
    unsigned char* key = malloc(FIORINO_KEY_SIZE);
    // https://crypto.stackexchange.com/questions/37629/pbkdf2-without-salt-on-16-digit-password
    // We really cannot do without a salt
    unsigned char* salt = malloc(FIORINO_PBKDF2_SALT_SIZE);
    unsigned char* iv = NULL;
    unsigned char tag[FIORINO_GCM_TAG_SIZE] = {0};
    char* fullpath = NULL;
    char* fullpath_tmp = NULL;
    unsigned char* serialized = NULL;
    int result = 1;
    if (contents == NULL || ciphertext == NULL || key == NULL || salt == NULL)
        goto cleanup_encrypt;
    if (fread(contents, 1, (size_t)fsize, file->file) != (size_t)fsize)
        goto cleanup_encrypt;
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    int rc = RAND_priv_bytes(salt, FIORINO_PBKDF2_SALT_SIZE);
    if (rc != 1) { result = 2; goto cleanup_encrypt; }
    // https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
    rc = PKCS5_PBKDF2_HMAC(
        vault->passphrase, -1,
        salt, FIORINO_PBKDF2_SALT_SIZE,
        FIORINO_PBKDF2_ITERATIONS, EVP_sha256(), // https://stackoverflow.com/a/63201320
        FIORINO_KEY_SIZE, key
    );
    if (rc != 1) { result = 2; goto cleanup_encrypt; }
    iv = malloc(FIORINO_IV_SIZE);
    if (iv == NULL) goto cleanup_encrypt;
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    rc = RAND_priv_bytes(iv, FIORINO_IV_SIZE);
    if (rc != 1) { result = 2; goto cleanup_encrypt; }
    int encryptrc = encrypt(contents, fsize, key, iv, ciphertext, tag);
    if (encryptrc < 0) { result = 2; goto cleanup_encrypt; }

    /// \todo check the goddamn filename and vault name
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t root_path;
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, WRDE_NOCMD) != 0)
        goto cleanup_encrypt;
    fullpath = output_path(root_path.we_wordv[0], vault->vault, file->name, ".enc");
    fullpath_tmp = output_path(root_path.we_wordv[0], vault->vault, file->name, ".enc.tmp");
    wordfree(&root_path);
    if (fullpath == NULL || fullpath_tmp == NULL) goto cleanup_encrypt;

    struct FiorinoBundle bundle = {
        .ciphertext_len = (size_t)encryptrc,
        .ciphertext = ciphertext,
    };
    memcpy(bundle.iv, iv, sizeof bundle.iv);
    memcpy(bundle.salt, salt, sizeof bundle.salt);
    memcpy(bundle.tag, tag, sizeof bundle.tag);
    size_t serialized_len = 0;
    serialized = serialize_bundle(&bundle, &serialized_len);
    if (serialized == NULL) goto cleanup_encrypt;

    FILE* file_enc = open_output_file(fullpath_tmp);
    if (file_enc == NULL) {
        /// \todo log with errno
        goto cleanup_encrypt;
    }
    // Locking the files: https://stackoverflow.com/a/7573369/15888601
    // Unlocking the files: https://stackoverflow.com/a/33939497/15888601

    write_output_file(file_enc, serialized, serialized_len);

    if (rename(fullpath_tmp, fullpath) == -1) {
        /// \todo log with errno
        /// \todo cleanup
        goto cleanup_encrypt;
    }

    result = 0;

cleanup_encrypt:
    if (key != NULL) {
        OPENSSL_cleanse(key, FIORINO_KEY_SIZE);
        free(key);
    }
    if (contents != NULL) {
        OPENSSL_cleanse(contents, contents_len);
        free(contents);
    }
    OPENSSL_cleanse(tag, sizeof tag);
    free(salt);
    free(iv);
    free(ciphertext);
    free(serialized);
    free(fullpath_tmp);
    free(fullpath);
    return result;
}

int fiorino_decrypt(struct Vault* vault, char* file) {
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    char* fullpath = runtime_setup_file(vault, file);
    if (fullpath == NULL) return 1;

    FILE* file_enc = open_input_file(fullpath);
    if (file_enc == NULL) {
        /// \todo log
        free(fullpath);
        return 1;
    }
    long long fsize = file_size(file_enc); // Very needed to check the file is not a crafted bomb payload
    if (fsize < 0 || fsize > FIORINO_MAX_ENCRYPTED_SIZE) {
        fclose(file_enc);
        free(fullpath);
        return 1;
    }
    unsigned char* bundled = malloc((size_t)fsize);
    if (bundled == NULL) {
        fclose(file_enc);
        free(fullpath);
        return 1;
    }
    size_t bytes_read = fread(bundled, 1, (size_t)fsize, file_enc);
    fclose(file_enc);
    if (bytes_read != (size_t)fsize) {
        free(bundled);
        free(fullpath);
        return 1;
    }
    struct FiorinoBundle* bundle = deserialize_bundle(bundled, (size_t)fsize);
    free(bundled);
    if (bundle == NULL) {
        free(fullpath);
        return 1;
    }

    unsigned char* key = malloc(FIORINO_KEY_SIZE);
    unsigned char* plaintext = NULL;
    size_t plaintext_size = 0;
    int result = 1;
    if (key == NULL) goto cleanup_decrypt;
    int rc = PKCS5_PBKDF2_HMAC(
        vault->passphrase, -1,
        bundle->salt, FIORINO_PBKDF2_SALT_SIZE,
        FIORINO_PBKDF2_ITERATIONS, EVP_sha256(), // https://stackoverflow.com/a/63201320
        FIORINO_KEY_SIZE, key
    );
    if (rc != 1) { result = 2; goto cleanup_decrypt; }

    plaintext_size = bundle->ciphertext_len ? bundle->ciphertext_len : 1;
    plaintext = malloc(plaintext_size);
    if (plaintext == NULL) goto cleanup_decrypt;
    int decryptrc = decrypt(
        bundle->ciphertext, bundle->ciphertext_len, bundle->tag,
        key, bundle->iv, plaintext
    );
    if (decryptrc == -1) {
        /// \todo log
        goto cleanup_decrypt;
    }

    FILE* exported = open_output_file(file);
    if (exported == NULL) {
        /// \todo log
        goto cleanup_decrypt;
    }
    write_output_file(exported, plaintext, (size_t)decryptrc);

    result = 0;

cleanup_decrypt:
    if (key != NULL) {
        OPENSSL_cleanse(key, FIORINO_KEY_SIZE);
        free(key);
    }
    if (plaintext != NULL) {
        OPENSSL_cleanse(plaintext, plaintext_size);
        free(plaintext);
    }
    free(fullpath);
    free(bundle);

    return result;
}
