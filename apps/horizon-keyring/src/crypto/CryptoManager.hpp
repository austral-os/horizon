#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace horizon::secrets::crypto
{
    class CryptoManager
    {
    public:
        CryptoManager() = default;
        static CryptoManager& instance();

        /**
         * @brief Derives a 256-bit key from a password and salt using Argon2id.
         */
        std::vector<uint8_t> derive_key(const std::string& password, const std::vector<uint8_t>& salt);

        /**
         * @brief Encrypts data using AES-256-GCM.
         * @return A vector containing IV + Tag + Ciphertext.
         */
        std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, const std::vector<uint8_t>& key);

        /**
         * @brief Decrypts data using AES-256-GCM.
         */
        std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encrypted_data, const std::vector<uint8_t>& key);

    private:
    };
}
