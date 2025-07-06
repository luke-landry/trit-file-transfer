#ifndef CRYPTO_H
#define CRYPTO_H

#include <vector>
#include <cstdint>
#include <string>
#include <array>
#include <utility>
#include <sodium.h>

// Using namespace instead of class to group cryptographic functions to encapsulate encryption implementation logic
namespace crypto {

inline constexpr std::size_t NONCE_SIZE = crypto_secretbox_NONCEBYTES;
inline constexpr std::size_t SALT_SIZE = crypto_pwhash_SALTBYTES;
inline constexpr std::size_t KEY_SIZE = crypto_secretstream_xchacha20poly1305_KEYBYTES;
inline constexpr std::size_t HEADER_SIZE = crypto_secretstream_xchacha20poly1305_HEADERBYTES;

// Number of extra bytes added by encryption
inline constexpr std::size_t ENCRYPTION_ADDITIONAL_BYTES = crypto_secretstream_xchacha20poly1305_ABYTES;

inline constexpr char HANDSHAKE_TAG_LITERAL[] = "trit_bonjour";
inline constexpr std::size_t HANDSHAKE_TAG_SIZE = sizeof(HANDSHAKE_TAG_LITERAL) - 1;  // exclude null terminator
inline constexpr std::size_t HANDSHAKE_CIPHERTEXT_SIZE = HANDSHAKE_TAG_SIZE + crypto_secretbox_MACBYTES;

class Nonce {
    public:
        Nonce(); // generates random nonce
        Nonce(const std::array<uint8_t, NONCE_SIZE>& buffer);
        const uint8_t* data() const noexcept;
        std::size_t size() const noexcept;

    private:
        std::array<uint8_t, NONCE_SIZE> data_;
};

class Salt {
    public:
        Salt(); // generates random salt
        Salt(const std::array<uint8_t, SALT_SIZE>& buffer);
        const uint8_t* data() const noexcept;
        std::size_t size() const noexcept;

    private:
        std::array<uint8_t, SALT_SIZE> data_;
};

class Key {
    public:
        Key(const std::string& password, const Salt& salt);
        const uint8_t* data() const noexcept;
        std::size_t size() const noexcept;

    private:
        std::array<uint8_t, KEY_SIZE> data_;
};

class Encryptor {
    public:
        explicit Encryptor(const Key& key);

        // Returns the stream header (must be sent to receiver)
        const std::array<uint8_t, HEADER_SIZE>& header() const noexcept;

        // Encrypts a buffer with output size guaranteed to be len + ENCRYPTION_ADDITIONAL_BYTES
        void encrypt(const uint8_t* data, std::size_t len, uint8_t* output, std::size_t* out_len, bool is_final = false);

    private:
        crypto_secretstream_xchacha20poly1305_state state_;
        std::array<uint8_t, HEADER_SIZE> header_;
};

class Decryptor {
    public:
        // Initializes decryption using the shared key and the received header
        Decryptor(const Key& key, const std::array<uint8_t, HEADER_SIZE>& header);

        // Decrypts a buffer with output size guaranteed to be len - ENCRYPTION_ADDITIONAL_BYTES
        void decrypt(const uint8_t* input, std::size_t len, uint8_t* output, std::size_t* out_len, bool& is_final);

    private:
        crypto_secretstream_xchacha20poly1305_state state_;
};


void init_sodium();

// Creates a nonce and encrypts the handshake tag with the provided key
std::pair<Nonce, std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE>> encrypt_handshake_tag(const Key& key);

// Attempts to decrypt and verify the provided ciphertext of the handshake tag given a key and nonce
bool verify_handshake_tag(const Key& key,
                          const Nonce& nonce,
                          const std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE>& ciphertext);

} // namespace crypto


#endif