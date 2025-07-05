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

void init_sodium();

// Creates a nonce and encrypts the handshake tag with the provided key
std::pair<Nonce, std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE>> encrypt_handshake_tag(const Key& key);

// Attempts to decrypt and verify the provided ciphertext of the handshake tag given a key and nonce
bool verify_handshake_tag(const Key& key,
                          const Nonce& nonce,
                          const std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE>& ciphertext);

} // namespace crypto


#endif