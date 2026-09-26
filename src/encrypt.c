/** \file encrypt.c
 * \brief AES-256-GCM encryption and authenticated decryption.
 * \note Ciphertext and the GCM tag are returned separately.
 */
#include <encrypt.h>

#include <openssl/evp.h>
#include <openssl/crypto.h>

int encrypt(const unsigned char *plaintext, int plaintext_len,
            const unsigned char *key, const unsigned char *iv,
            unsigned char *ciphertext,
            unsigned char tag[FIORINO_GCM_TAG_SIZE])
{
    if (plaintext == NULL || plaintext_len < 0 || key == NULL || iv == NULL ||
        ciphertext == NULL || tag == NULL) return -1;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) return -1;
    int len = 0, ciphertext_len = 0, result = -1;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, FIORINO_IV_SIZE,
                            NULL) != 1 ||
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1 ||
        EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
        goto done;
    ciphertext_len = len;
    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &len) != 1)
        goto done;
    ciphertext_len += len;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, FIORINO_GCM_TAG_SIZE,
                            tag) != 1)
        goto done;
    result = ciphertext_len;

done:
    EVP_CIPHER_CTX_free(ctx);
    return result;
}

int decrypt(const unsigned char *ciphertext, int ciphertext_len,
            const unsigned char tag[FIORINO_GCM_TAG_SIZE],
            const unsigned char *key, const unsigned char *iv,
            unsigned char *plaintext)
{
    if (ciphertext == NULL || ciphertext_len < 0 || tag == NULL ||
        key == NULL || iv == NULL || plaintext == NULL) return -1;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) return -1;
    int len = 0, plaintext_len = 0, result = -1;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, FIORINO_IV_SIZE,
                            NULL) != 1 ||
        EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv) != 1 ||
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
        goto done;
    plaintext_len = len;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, FIORINO_GCM_TAG_SIZE,
                            (void *)tag) != 1 ||
        EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len, &len) != 1)
        goto done;
    result = plaintext_len + len;

done:
    if (result < 0 && plaintext_len > 0)
        OPENSSL_cleanse(plaintext, (size_t)plaintext_len);
    EVP_CIPHER_CTX_free(ctx);
    return result;
}
