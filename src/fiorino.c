#include <fiorino.h>
#include <bundle.h>
#include <encrypt.h>
#include <fiorino_log.h>

#include <fcntl.h> // open
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <sys/file.h>
#include <string.h>
#include <wordexp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/ui.h>
#include <openssl/crypto.h>

#define FIORINO_MAX_ENCRYPTED_SIZE (FIORINO_MAX_FILE_SIZE \
                                    + FIORINO_IV_SIZE \
                                    + FIORINO_GCM_TAG_SIZE \
                                    + FIORINO_PBKDF2_SALT_SIZE )
#define FIORINO_AUTH_FILENAME ".auth"

_Static_assert(FIORINO_MAX_FILE_SIZE < SIZE_MAX,
               "FIORINO_MAX_FILE_SIZE must leave room for a terminating byte");

int fiorino_read_password(char *buffer, size_t buffer_size, int verify) {
    if (buffer == NULL || buffer_size == 0 || buffer_size > INT_MAX)
        return -1;

    return UI_UTIL_read_pw_string(buffer, (int)buffer_size, "Password: ", verify != 0);
}

void fiorino_cleanse(void* buffer, size_t size) {
    OPENSSL_cleanse(buffer, size);
}

/** \brief Returns the length of the file in bytes
 * \warning It leaves the cursor at the beginning of the file
 */
long long file_size(FILE*);
/** \brief Verifies that the system and runtime conditions are met
 * \note Must be strictly idemponent (e.g. don't delete .fiorino/ contents by mistake)
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
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, WRDE_NOCMD) != 0) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not expand vault root path");
        return 1;
    }
    // https://stackoverflow.com/a/7430262/15888601
    struct stat st = {0};
    if (stat(vault_root_path.we_wordv[0], &st) == -1) {
        if (mkdir(vault_root_path.we_wordv[0], FIORINO_VAULT_DIR_PERM) == -1) {
            fiorino_log_errno(FIORINO_LOG_ERROR,
                              "could not create vault root directory", vault_root_path.we_wordv[0]);
            wordfree(&vault_root_path);
            return 1;
        } else {
            fiorino_log_message(FIORINO_LOG_INFO,
                                "vault root directory created: '%s'",
                                vault_root_path.we_wordv[0]);
        }
    } else if (FIORINO_VAULT_DIR_PERM != st.st_mode) {
        if (chmod(vault_root_path.we_wordv[0], FIORINO_VAULT_DIR_PERM) == -1) {
            fiorino_log_errno(FIORINO_LOG_ERROR,
                              "could not set vault root permissions", vault_root_path.we_wordv[0]);
            wordfree(&vault_root_path);
            return 1;
        }
    }
    wordfree(&vault_root_path);
    return 0;
}

/** \brief Idempotently prepares a vault directory and authenticates its password
 * \param vault The vault to be checked
 * \return Status code
 * \retval 0 vault is ready and authentication succeeded
 * \retval 1 setup or I/O failure
 * \retval FIORINO_AUTHENTICATION_FAILED password does not match the vault
 */
int vault_directory_exists(struct Vault* vault) {
    if (strstr(vault->vault, "/") || strcmp(vault->vault, "..") == 0 ||
        strcmp(vault->vault, ".") == 0) {
        fiorino_log_message(FIORINO_LOG_WARNING, "invalid vault name");
        return 1;
    }
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t vault_root_path;
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &vault_root_path, WRDE_NOCMD) != 0) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not expand vault root path");
        return 1;
    }
    int vaultpath_alloc = strlen(vault_root_path.we_wordv[0]) + 1
        + strlen(vault->vault) + 1;
    if (vaultpath_alloc > PATH_MAX) {
        fiorino_log_message(FIORINO_LOG_WARNING, "vault path is too long");
        wordfree(&vault_root_path);
        return 1;
    }
    char* vaultpath = malloc(vaultpath_alloc);
    if (vaultpath == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate vault path", vault->vault);
        wordfree(&vault_root_path);
        return 1;
    }
    sprintf(vaultpath, "%s/%s",
            vault_root_path.we_wordv[0], vault->vault);
    wordfree(&vault_root_path);

    // https://stackoverflow.com/a/7430262/15888601
    struct stat st = {0};
    if (lstat(vaultpath, &st) == -1) {
        if (errno != ENOENT) {
            fiorino_log_errno(FIORINO_LOG_ERROR,
                              "could not inspect vault directory", vaultpath);
            free(vaultpath);
            return 1;
        }
        if (mkdir(vaultpath, FIORINO_VAULT_DIR_PERM) == -1) {
            fiorino_log_errno(FIORINO_LOG_ERROR, "could not create vault directory", vaultpath);
            free(vaultpath);
            return 1;
        } else {
            fiorino_log_message(FIORINO_LOG_INFO,
                                "vault created: '%s'", vault->vault);
        }
    } else if (S_ISLNK(st.st_mode) || !S_ISDIR(st.st_mode)) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "vault path is not a real directory: '%s'", vaultpath);
        free(vaultpath);
        return 1;
    } else if ((st.st_mode & 07777) != FIORINO_VAULT_DIR_PERM) {
        if (chmod(vaultpath, FIORINO_VAULT_DIR_PERM) == -1) {
            fiorino_log_errno(FIORINO_LOG_ERROR, "could not set vault permissions", vaultpath);
            free(vaultpath);
            return 1;
        }
    }

    int auth_failed = 0;
    char auth_path[PATH_MAX];
    int auth_path_size = snprintf(auth_path, sizeof auth_path, "%s/%s",
                                  vaultpath, FIORINO_AUTH_FILENAME);
    if (auth_path_size < 0) {
        fiorino_log_message(FIORINO_LOG_ERROR,
                            "could not format vault authentication path");
        auth_failed = 1;
    } else if ((size_t)auth_path_size >= sizeof auth_path) {
        fiorino_log_message(FIORINO_LOG_ERROR,
                            "the auth path is too long");
        auth_failed = 1;
    } else {
        int authfd = open(auth_path, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC |
                          O_NOFOLLOW, FIORINO_VAULT_PERM);
        unsigned char salt[FIORINO_PBKDF2_SALT_SIZE] = {0};
        unsigned char derived[FIORINO_KEY_SIZE] = {0};
        if (authfd == -1) {
            if (errno != EEXIST) {
                fiorino_log_errno(FIORINO_LOG_ERROR,
                                  "could not create vault authentication record",
                                  auth_path);
                auth_failed = 1;
            } else if (errno == EEXIST) {
                unsigned char stored[FIORINO_KEY_SIZE] = {0};
                authfd = open(auth_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
                if (authfd == -1) {
                    fiorino_log_errno(FIORINO_LOG_DEBUG,
                                      "could not open to read", auth_path);
                    auth_failed = 1;
                } else if (read(authfd, salt, sizeof salt) != sizeof salt) {
                    fiorino_log_errno(FIORINO_LOG_DEBUG,
                                      "could not read salt", auth_path);
                    auth_failed = 1;
                } else if (read(authfd, stored, sizeof stored) != sizeof stored) {
                    fiorino_log_errno(FIORINO_LOG_DEBUG,
                                      "could not read stored hash", auth_path);
                    auth_failed = 1;
                } else if (PKCS5_PBKDF2_HMAC(
                            vault->passphrase, -1,
                            salt, sizeof salt,
                            FIORINO_PBKDF2_ITERATIONS, EVP_sha256(),
                            FIORINO_KEY_SIZE, derived) != 1) {
                    fiorino_log_message(FIORINO_LOG_DEBUG,
                                        "could not derive key from %s", auth_path);
                    auth_failed = 1;
                } else if (CRYPTO_memcmp(derived, stored, FIORINO_KEY_SIZE) != 0) {
                    fiorino_log_message(FIORINO_LOG_DEBUG,
                                        "the derived key does not correspond to the one in %s",
                                        auth_path);
                    auth_failed = FIORINO_AUTHENTICATION_FAILED;
                } else {
                    fiorino_log_message(FIORINO_LOG_DEBUG,
                                        "the derived key corresponds to the one in %s",
                                        auth_path);
                    auth_failed = 0;
                }
                if (authfd >= 0) {
                    if (close(authfd) == -1) {
                        fiorino_log_errno(FIORINO_LOG_ERROR,
                                  "could not close vault authentication record",
                                  auth_path);
                        auth_failed = 1;
                    }
                }
                OPENSSL_cleanse(stored, sizeof stored);
            }
        } else {
            if (RAND_priv_bytes(salt, sizeof salt) != 1) {
                fiorino_log_message(FIORINO_LOG_ERROR,
                                    "could not generate authentication salt");
                auth_failed = 1;
            } else if (PKCS5_PBKDF2_HMAC(
                           vault->passphrase, -1, salt, sizeof salt,
                           FIORINO_PBKDF2_ITERATIONS, EVP_sha256(),
                           sizeof derived, derived) != 1) {
                fiorino_log_message(FIORINO_LOG_ERROR,
                                    "could not derive vault authentication verifier");
                auth_failed = 1;
            } else if (write(authfd, salt, sizeof salt) != (ssize_t)sizeof salt) {
                fiorino_log_errno(FIORINO_LOG_ERROR,
                                  "could not write authentication salt", auth_path);
                auth_failed = 1;
            } else if (write(authfd, derived, sizeof derived) !=
                       (ssize_t)sizeof derived) {
                fiorino_log_errno(FIORINO_LOG_ERROR,
                                  "could not write authentication verifier", auth_path);
                auth_failed = 1;
            } else if (fsync(authfd) == -1) {
                fiorino_log_errno(FIORINO_LOG_DEBUG,
                                  "could not (f)sync to disk", auth_path);
                auth_failed = 1;
            } // Maybe this is a bit paranoid

            if (auth_failed) {
                close(authfd);
                unlink(auth_path);
            } else if (close(authfd) == -1) {
                fiorino_log_errno(FIORINO_LOG_ERROR,
                                  "could not close vault authentication record",
                                  auth_path);
                unlink(auth_path);
                auth_failed = 1;
            } else {
                fiorino_log_message(FIORINO_LOG_INFO,
                                    "vault authentication record created: '%s'",
                                    vault->vault);
            }
        }
        OPENSSL_cleanse(salt, sizeof salt);
        OPENSSL_cleanse(derived, sizeof derived);
    }

    free(vaultpath);
    return auth_failed;
}

/** \brief Idempotently creates a vault directory with correct permissions
 * \param vault The vault whose user is to be checked
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int vault_user_corresponds(struct Vault* vault) {
    if (vault->user != geteuid()) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "vault user does not match the effective user");
        return 1;
    }
    return 0;
}

int runtime_setup() {
    return vault_root_directory_exists();
}

int runtime_setup_vault(struct Vault* vault) {
    int result = vault_directory_exists(vault);
    if (result != 0)
        return result;
    return vault_user_corresponds(vault);
}

/** \see https://stackoverflow.com/a/4553076/15888601 */
int is_regular_file(const char *path) {
    struct stat path_stat;
    /// \see https://stackoverflow.com/a/2635948/15888601
    if (lstat(path, &path_stat) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING, "could not inspect encrypted file", path);
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
        fiorino_log_message(FIORINO_LOG_WARNING, "invalid output name");
        return 1;
    }
    return 0;
}

/** \brief Validates the open plaintext file and obtains its size
 * \note Leaves the stream positioned at the beginning for encryption
 * \param locked Whether the underlying file is currently locked or not
 * \note The locking is performed with flock
 * \note It will reliably fail on files larger than FIORINO_MAX_FILE_SIZE
 * \return Status code
 * \retval 0 success
 * \retval 1 failure
 */
int runtime_setup_plaintext_file(FILE* input, long long* size, int* locked) {
    if (input == NULL || size == NULL || locked == NULL) {
        fiorino_log_message(FIORINO_LOG_WARNING, "missing plaintext stream or size output");
        return 1;
    }
    int fd = fileno(input);
    *locked = 0;
    struct stat st;
    if (fd < 0) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not lock file descriptor", "plaintext stream");
        return 1;
    } else if (flock(fd, LOCK_SH) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not lock plaintext stream", "plaintext stream");
        return 1;
    }
    *locked = 1;
    if (fseek(input, 0, SEEK_SET) != 0) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not rewind plaintext stream", "plaintext stream");
        goto fail;
    } else if (fstat(fd, &st) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not retrieve plaintext statistics", "plaintext stream");
        goto fail;
    } else if (!S_ISREG(st.st_mode)) {
        fiorino_log_message(FIORINO_LOG_WARNING, "plaintext stream is not a regular file");
        goto fail;
    } else if (st.st_size < 0) {
        fiorino_log_message(FIORINO_LOG_WARNING, "plaintext stream has an invalid size");
        goto fail;
    } else if (st.st_size > FIORINO_MAX_FILE_SIZE) {
        fiorino_log_message(FIORINO_LOG_WARNING, "plaintext file exceeds the size limit");
        goto fail;
    }
    *size = st.st_size;
    return 0;

fail:
    if (*locked && flock(fd, LOCK_UN) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not unlock file descriptor", "plaintext stream");
    } else {
        *locked = 0;
    }
    return 1;
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
        fiorino_log_message(FIORINO_LOG_WARNING, "invalid encrypted file name");
        return NULL;
    }

    wordexp_t root_path;
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, WRDE_NOCMD) != 0) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not expand vault root path");
        return NULL;
    }
    char* fullpath = output_path(root_path.we_wordv[0], vault->vault, file, ".enc");
    wordfree(&root_path);
    if (fullpath == NULL) {
        return NULL;
    }
    int regular = is_regular_file(fullpath);
    if (regular < 0) {
        free(fullpath);
        return NULL;
    } else if (regular == 0) {
        fiorino_log_message(FIORINO_LOG_WARNING, "encrypted input is not a regular file");
        free(fullpath);
        return NULL;
    }
    return fullpath;
}

static char* output_path(const char* root, const char* vault,
                         const char* name, const char* suffix) {
    int length = snprintf(NULL, 0, "%s/%s/%s%s", root, vault, name, suffix);
    if (length < 0) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not format output path");
        return NULL;
    } else if (length + 4 > PATH_MAX) { // Could never fit ".tmp"
        fiorino_log_message(FIORINO_LOG_WARNING, "output path is too long");
        return NULL;
    }

    char* path = malloc((size_t)length + 1);
    if (path != NULL)
        snprintf(path, (size_t)length + 1, "%s/%s/%s%s",
                 root, vault, name, suffix);
    else
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate output path", name);
    return path;
}

static FILE* open_output_file_to_vault(const char* path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK,
                  FIORINO_VAULT_PERM);
    if (fd == -1) {
        fiorino_log_errno(errno == ELOOP ? FIORINO_LOG_WARNING : FIORINO_LOG_ERROR,
                          "could not open vault output", path);
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not inspect vault output", path);
        close(fd);
        return NULL;
    }
    if (!S_ISREG(st.st_mode)) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "vault output '%s' is not a regular file", path);
        close(fd);
        return NULL;
    }
    if (flock(fd, LOCK_EX) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not lock vault output", path);
        close(fd);
        return NULL;
    }
    if (fchmod(fd, FIORINO_VAULT_PERM) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not set vault output permissions", path);
        close(fd);
        return NULL;
    }
    if (ftruncate(fd, 0) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not truncate vault output", path);
        close(fd);
        return NULL;
    }
    FILE* output = fdopen(fd, "wb");
    if (output == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not create vault output stream", path);
        close(fd);
        return NULL; // Ok this is funny but I like it explicit
    }
    return output;
}

static FILE* open_output_file_to_user(const char* path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK,
                  FIORINO_VAULT_PERM); // If the file doesn't exist yet, set permission
    if (fd == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING, "could not open user output", path);
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not inspect user output", path);
        close(fd);
        return NULL;
    }
    if (!S_ISREG(st.st_mode)) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "user output '%s' is not a regular file", path);
        close(fd);
        return NULL;
    }
    if (flock(fd, LOCK_EX) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not lock user output", path);
        close(fd);
        return NULL;
    }
    if (ftruncate(fd, 0) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not truncate user output", path);
        close(fd);
        return NULL;
    }
    FILE* output = fdopen(fd, "wb");
    if (output == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not create user output stream", path);
        close(fd);
        return NULL;
    }
    return output;
}

static FILE* open_input_file(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) {
        fiorino_log_errno(FIORINO_LOG_WARNING, "could not open encrypted file", path);
        return NULL;
    }

    struct stat st;
    const off_t min_size = FIORINO_IV_SIZE + FIORINO_PBKDF2_SALT_SIZE +
                           FIORINO_GCM_TAG_SIZE;
    if (fstat(fd, &st) != 0) {
        int error = errno;
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not inspect encrypted file", path);
        close(fd);
        errno = error;
        return NULL;
    }
    if (!S_ISREG(st.st_mode) || st.st_uid != geteuid() ||
        st.st_size < min_size ||
        st.st_size > min_size + FIORINO_MAX_FILE_SIZE) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "encrypted file '%s' has invalid type, owner, or size", path);
        close(fd);
        errno = EINVAL;
        return NULL;
    }

    FILE* input = fdopen(fd, "rb");
    if (input == NULL) {
        int error = errno;
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not create encrypted input stream", path);
        close(fd);
        errno = error;
    }
    return input;
}

/**
 * \note It closes the output stream before returning
 * \return status code
 * \retval 0 success
 * \retval -1 failure
 */
static int write_close_output_file(FILE* output, const unsigned char* data,
                              size_t length) {
    int failed = 0;
    if (fwrite(data, 1, length, output) != length) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not write complete output");
        failed = 1;
    }
    if (fflush(output) == EOF) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not flush", "output stream");
        failed = 1;
    }
    if (flock(fileno(output), LOCK_UN) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not unlock", "output stream");
        failed = 1;
    }
    if (fclose(output) == EOF) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not close", "output stream");
        failed = 1;
    }
    return failed ? -1 : 0;
}

int fiorino_encrypt_stream(struct Vault* vault, struct FileToEncrypt* file) {
    fiorino_log_message(FIORINO_LOG_INFO,
                        "encryption requested for vault '%s', file '%s'",
                        vault->vault, file->name);
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    if (runtime_setup_output_name(file->name)) return 1;

    long long fsize;
    int file_locked = 0;
    if (runtime_setup_plaintext_file(file->file, &fsize, &file_locked)) return 1;
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
    if (contents == NULL || ciphertext == NULL || key == NULL || salt == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "encryption buffers");
        goto cleanup_encrypt;
    }
    size_t total_read = 0;

    /** \warning This casting is only safe thanks to runtime_setup_plaintext_file
     *           guaranteeing fsize <= FIORINO_MAX_FILE_SIZE and said constant
     *           fitting within a size_t as per static assertions
     */
    size_t ufsize = (size_t)fsize;
    while (total_read < ufsize) {
        size_t current_read = fread(contents + total_read,
                                    1, ufsize - total_read, file->file);
        if (current_read == 0) {
            fiorino_log_message(FIORINO_LOG_ERROR,
                                "could not read complete plaintext");
            goto cleanup_encrypt;
        }
        total_read += current_read;
    }
    if (total_read > ufsize || fgetc(file->file) != EOF) {
        fiorino_log_message(FIORINO_LOG_ERROR,
                            "the plaintext file grew beyond its measured size");
        goto cleanup_encrypt;
    }
    if (ferror(file->file)) {
        fiorino_log_message(FIORINO_LOG_ERROR,
                            "failure while reading plaintext from file");
        goto cleanup_encrypt;
    }
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    int rc = RAND_priv_bytes(salt, FIORINO_PBKDF2_SALT_SIZE);
    if (rc != 1) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not generate encryption salt");
        result = 2; goto cleanup_encrypt;
    }
    // https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html#pbkdf2
    rc = PKCS5_PBKDF2_HMAC(
        vault->passphrase, -1,
        salt, FIORINO_PBKDF2_SALT_SIZE,
        FIORINO_PBKDF2_ITERATIONS, EVP_sha256(), // https://stackoverflow.com/a/63201320
        FIORINO_KEY_SIZE, key
    );
    if (rc != 1) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not derive encryption key");
        result = 2; goto cleanup_encrypt;
    }
    iv = malloc(FIORINO_IV_SIZE);
    if (iv == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "encryption IV");
        goto cleanup_encrypt;
    }
    // https://docs.openssl.org/3.5/man3/RAND_bytes/
    rc = RAND_priv_bytes(iv, FIORINO_IV_SIZE);
    if (rc != 1) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not generate encryption IV");
        result = 2; goto cleanup_encrypt;
    }
    int encryptrc = fiorino_encrypt_gcm(contents, fsize, key, iv, ciphertext, tag);
    if (encryptrc < 0) {
        result = 2; goto cleanup_encrypt;
    }

    /// \todo check the goddamn filename and vault name
    // https://stackoverflow.com/a/15991337/15888601
    wordexp_t root_path;
    if (wordexp(FIORINO_VAULT_ROOT_DIR, &root_path, WRDE_NOCMD) != 0) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not expand vault root path");
        goto cleanup_encrypt;
    }
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

    FILE* file_enc = open_output_file_to_vault(fullpath_tmp);
    if (file_enc == NULL) goto cleanup_encrypt;
    // Locking the files: https://stackoverflow.com/a/7573369/15888601
    // Unlocking the files: https://stackoverflow.com/a/33939497/15888601

    if (write_close_output_file(file_enc, serialized, serialized_len) == -1) {
        goto cleanup_encrypt;
    }

    if (rename(fullpath_tmp, fullpath) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not replace encrypted file", fullpath);
        // if (remove(fullpath_tmp) == -1) {
        if (unlink(fullpath_tmp) == -1 && errno != ENOENT) {
            fiorino_log_errno(FIORINO_LOG_ERROR, "could not delete temporary", fullpath_tmp);
        }
        goto cleanup_encrypt;
    }

    result = 0;
    fiorino_log_message(FIORINO_LOG_INFO,
                        "encryption succeeded for vault '%s', file '%s'",
                        vault->vault, file->name);

cleanup_encrypt:
    if (file_locked && flock(fileno(file->file), LOCK_UN) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not unlock file descriptor", "plaintext stream");
        result = result ? result : 1;
    }
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

int fiorino_decrypt(struct Vault* vault, const char* file) {
    fiorino_log_message(FIORINO_LOG_INFO,
                        "decryption requested for vault '%s', file '%s'",
                        vault->vault, file);
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    char* fullpath = runtime_setup_file(vault, file);
    if (fullpath == NULL) return 1;

    FILE* file_enc = open_input_file(fullpath);
    if (file_enc == NULL) {
        free(fullpath);
        return 1;
    }
    long long fsize = file_size(file_enc); // Very needed to check the file is not a crafted bomb payload
    if (fsize < 0 || fsize > FIORINO_MAX_ENCRYPTED_SIZE) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "encrypted file '%s' has an invalid size", fullpath);
        fclose(file_enc);
        free(fullpath);
        return 1;
    }
    unsigned char* bundled = malloc((size_t)fsize);
    if (bundled == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "encrypted input buffer");
        fclose(file_enc);
        free(fullpath);
        return 1;
    }
    size_t bytes_read = fread(bundled, 1, (size_t)fsize, file_enc);
    fclose(file_enc);
    if (bytes_read != (size_t)fsize) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not read complete encrypted file");
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
    if (key == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "decryption key");
        goto cleanup_decrypt;
    }
    int rc = PKCS5_PBKDF2_HMAC(
        vault->passphrase, -1,
        bundle->salt, FIORINO_PBKDF2_SALT_SIZE,
        FIORINO_PBKDF2_ITERATIONS, EVP_sha256(), // https://stackoverflow.com/a/63201320
        FIORINO_KEY_SIZE, key
    );
    if (rc != 1) {
        fiorino_log_message(FIORINO_LOG_ERROR, "could not derive decryption key");
        result = 2; goto cleanup_decrypt;
    }

    plaintext_size = bundle->ciphertext_len ? bundle->ciphertext_len : 1;
    plaintext = malloc(plaintext_size);
    if (plaintext == NULL) {
        fiorino_log_errno(FIORINO_LOG_ERROR, "could not allocate", "plaintext output buffer");
        goto cleanup_decrypt;
    }
    int decryptrc = fiorino_decrypt_gcm(
        bundle->ciphertext, bundle->ciphertext_len, bundle->tag,
        key, bundle->iv, plaintext
    );
    if (decryptrc == -1) {
        goto cleanup_decrypt;
    }

    FILE* exported = open_output_file_to_user(file);
    if (exported == NULL) goto cleanup_decrypt;
    if (write_close_output_file(exported, plaintext, (size_t)decryptrc) == -1) {
        goto cleanup_decrypt;
    }

    result = 0;
    fiorino_log_message(FIORINO_LOG_INFO,
                        "decryption succeeded for vault '%s', file '%s'",
                        vault->vault, file);

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

/** \brief Reports the number of hard links to the inode of the specified file
 *  \retval -1 Failure to retrieve the information
 *  \return The number of hard links
 */
nlink_t fiorino_hardlinks(const char* path) {
    struct stat sb;
    if (stat(path, &sb) == -1) {
        fiorino_log_errno(FIORINO_LOG_WARNING,
                          "could not stat the file", path);
        return -1;
    }
    /// \todo informationally log the paths of the other links, if possible
    return sb.st_nlink;
}

int fiorino_delete(struct Vault* vault, const char* file) {
    fiorino_log_message(FIORINO_LOG_INFO,
                        "deletion requested for vault '%s', file '%s'",
                        vault->vault, file);
    if (runtime_setup()) return 1;
    if (runtime_setup_vault(vault)) return 1;
    char* fullpath = runtime_setup_file(vault, file);
    if (fullpath == NULL) return 1;
    int result = 1;

    nlink_t links = fiorino_hardlinks(fullpath);
    if (links > 1) {
        fiorino_log_message(FIORINO_LOG_WARNING,
                            "%lu hard links exist to %s, so the inode will persist",
                            links, fullpath);
        result = 2;
        goto cleanup_delete;
    }

    /// \todo delete/shred the content
    if (unlink(fullpath) == -1) {
        fiorino_log_errno(FIORINO_LOG_ERROR,
                          "could not unlink %s",
                          fullpath);
        goto cleanup_delete;
    }

    result = 0;
    fiorino_log_message(FIORINO_LOG_INFO,
                        "deletion succeeded for vault '%s', file '%s'",
                        vault->vault, file);

cleanup_delete:
    free(fullpath);

    return result;
}
