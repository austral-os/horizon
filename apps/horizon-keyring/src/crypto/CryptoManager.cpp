#include "CryptoManager.hpp"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace horizon::secrets::crypto
{
    CryptoManager& CryptoManager::instance()
    {
        static CryptoManager instance;
        return instance;
    }

    std::vector<uint8_t> CryptoManager::derive_key(const std::string& password, const std::vector<uint8_t>& salt)
    {
        std::vector<uint8_t> key(32); // 256 bits
        EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
        if (!kdf) throw std::runtime_error("Argon2id KDF not found");

        EVP_KDF_CTX *ctx = EVP_KDF_CTX_new(kdf);
        EVP_KDF_free(kdf);
        if (!ctx) throw std::runtime_error("Failed to create KDF context");

        OSSL_PARAM params[6];
        uint32_t threads = 1;
        uint32_t memory = 65536; // 64 MB
        uint32_t iterations = 3;

        params[0] = OSSL_PARAM_construct_octet_string("pass", (void*)password.data(), password.size());
        params[1] = OSSL_PARAM_construct_octet_string("salt", (void*)salt.data(), salt.size());
        params[2] = OSSL_PARAM_construct_uint32("iter", &iterations);
        params[3] = OSSL_PARAM_construct_uint32("mem", &memory);
        params[4] = OSSL_PARAM_construct_uint32("threads", &threads);
        params[5] = OSSL_PARAM_construct_end();

        if (EVP_KDF_derive(ctx, key.data(), key.size(), params) <= 0) {
            unsigned long err = ERR_get_error();
            char err_buf[256];
            ERR_error_string_n(err, err_buf, sizeof(err_buf));
            EVP_KDF_CTX_free(ctx);
            throw std::runtime_error("Argon2id key derivation failed: " + std::string(err_buf));
        }

        EVP_KDF_CTX_free(ctx);
        return key;
    }

    std::vector<uint8_t> CryptoManager::encrypt(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key)
    {
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        std::vector<uint8_t> iv(12);
        RAND_bytes(iv.data(), iv.size());

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Encryption init failed");
        }

        std::vector<uint8_t> ciphertext(plaintext.size());
        int len;
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Encryption update failed");
        }

        if (EVP_EncryptFinal_ex(ctx, nullptr, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Encryption final failed");
        }

        std::vector<uint8_t> tag(16);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to get GCM tag");
        }

        EVP_CIPHER_CTX_free(ctx);

        // Result: IV (12) + Tag (16) + Ciphertext
        std::vector<uint8_t> result;
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), tag.begin(), tag.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());

        return result;
    }

    std::vector<uint8_t> CryptoManager::decrypt(const std::vector<uint8_t>& encrypted_data, const std::vector<uint8_t>& key)
    {
        if (encrypted_data.size() < 28) throw std::runtime_error("Invalid encrypted data size");

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        const uint8_t* iv = encrypted_data.data();
        const uint8_t* tag = encrypted_data.data() + 12;
        const uint8_t* ciphertext = encrypted_data.data() + 28;
        size_t ciphertext_len = encrypted_data.size() - 28;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption init failed");
        }

        std::vector<uint8_t> plaintext(ciphertext_len);
        int len;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption update failed");
        }

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set GCM tag");
        }

        if (EVP_DecryptFinal_ex(ctx, nullptr, &len) <= 0) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption failed (integrity check failed)");
        }

        EVP_CIPHER_CTX_free(ctx);
        return plaintext;
    }
}
