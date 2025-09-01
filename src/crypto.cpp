#include "crypto.h"
#include <cstring>
#include <sodium.h>
#include <stdexcept>

// Anonymous namespace to hide internal crypto utility functions
namespace {

std::array<uint8_t, crypto::NONCE_SIZE> generate_nonce_bytes() {
  std::array<uint8_t, crypto::NONCE_SIZE> nonce{};
  randombytes_buf(nonce.data(), nonce.size());
  return nonce;
}

std::array<uint8_t, crypto::SALT_SIZE> generate_salt_bytes() {
  std::array<uint8_t, crypto::SALT_SIZE> salt{};
  randombytes_buf(salt.data(), salt.size());
  return salt;
}

std::array<uint8_t, crypto::KEY_SIZE>
derive_key_bytes(const std::string &password, const crypto::Salt &salt) {
  std::array<uint8_t, crypto::KEY_SIZE> key{};

  if (crypto_pwhash(key.data(), key.size(), password.c_str(), password.size(),
                    salt.data(), crypto_pwhash_OPSLIMIT_MODERATE,
                    crypto_pwhash_MEMLIMIT_MODERATE,
                    crypto_pwhash_ALG_DEFAULT) != 0) {
    throw std::runtime_error("Key derivation failed");
  }

  return key;
}

} // anonymous namespace

namespace crypto {

// Public crypto classes

// Nonce class

Nonce::Nonce() : data_(generate_nonce_bytes()) {}

Nonce::Nonce(const std::array<uint8_t, NONCE_SIZE> &buffer) : data_(buffer) {}

const uint8_t *crypto::Nonce::data() const noexcept { return data_.data(); }

std::size_t crypto::Nonce::size() const noexcept { return data_.size(); }

// Salt class

Salt::Salt() : data_(generate_salt_bytes()) {}

Salt::Salt(const std::array<uint8_t, SALT_SIZE> &buffer) : data_(buffer) {}

const uint8_t *Salt::data() const noexcept { return data_.data(); }

std::size_t Salt::size() const noexcept { return data_.size(); }

// Key class

Key::Key(const std::string &password, const Salt &salt)
    : data_(derive_key_bytes(password, salt)) {}

const uint8_t *Key::data() const noexcept { return data_.data(); }

std::size_t Key::size() const noexcept { return data_.size(); }

// Encryptor class

Encryptor::Encryptor(const Key &key) {
  if (crypto_secretstream_xchacha20poly1305_init_push(&state_, header_.data(),
                                                      key.data()) != 0) {
    throw std::runtime_error(
        "Encryptor: Failed to initialize stream encryption");
  }
}

const std::array<uint8_t, HEADER_SIZE> &Encryptor::header() const noexcept {
  return header_;
}

void Encryptor::encrypt(const uint8_t *input, std::size_t len, uint8_t *output,
                        std::size_t *out_len, bool is_final) {
  uint8_t tag = is_final ? crypto_secretstream_xchacha20poly1305_TAG_FINAL
                         : crypto_secretstream_xchacha20poly1305_TAG_MESSAGE;

  unsigned long long out_len_raw = 0;

  if (crypto_secretstream_xchacha20poly1305_push(
          &state_, output, &out_len_raw, input,
          static_cast<unsigned long long>(len), nullptr, 0, tag) != 0) {
    throw std::runtime_error("Encryptor: Encryption failed");
  }

  if (out_len_raw != (len + ENCRYPTION_ADDITIONAL_BYTES)) {
    throw std::runtime_error("Encryption output bad length");
  }

  if (out_len) {
    *out_len = static_cast<std::size_t>(out_len_raw);
  }
}

// Decryptor class

Decryptor::Decryptor(const Key &key,
                     const std::array<uint8_t, HEADER_SIZE> &header) {
  if (crypto_secretstream_xchacha20poly1305_init_pull(&state_, header.data(),
                                                      key.data()) != 0) {
    throw std::runtime_error(
        "Decryptor: Failed to initialize stream decryption");
  }
}

void Decryptor::decrypt(const uint8_t *input, std::size_t len, uint8_t *output,
                        std::size_t *out_len, bool &is_final) {
  if (len < ENCRYPTION_ADDITIONAL_BYTES) {
    throw std::invalid_argument("Decryptor: Ciphertext chunk too small");
  }

  unsigned long long out_len_raw = 0;
  uint8_t tag = 0;

  if (crypto_secretstream_xchacha20poly1305_pull(
          &state_, output, &out_len_raw, &tag, input,
          static_cast<unsigned long long>(len), nullptr, 0) != 0) {
    throw std::runtime_error(
        "Decryptor: Authentication failed or corrupt chunk");
  }

  if (out_len_raw != (len - ENCRYPTION_ADDITIONAL_BYTES)) {
    throw std::runtime_error("Decryption output bad length");
  }

  is_final = (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL);

  if (out_len) {
    *out_len = static_cast<std::size_t>(out_len_raw);
  }
}

// public crypto functions

void init_sodium() {
  if (sodium_init() < 0) {
    throw std::runtime_error("libsodium initialization failed");
  }
}

std::pair<Nonce, std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE>>
encrypt_handshake_tag(const Key &key) {
  Nonce nonce;

  // ciphertext length is the size of the handshake + the libsodium MAC tag
  std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE> ciphertext;

  if (crypto_secretbox_easy(
          ciphertext.data(),
          reinterpret_cast<const uint8_t *>(HANDSHAKE_TAG_LITERAL),
          HANDSHAKE_TAG_SIZE, nonce.data(), key.data()) != 0) {
    throw std::runtime_error("Failed to encrypt handshake tag");
  }

  return {std::move(nonce), std::move(ciphertext)};
}

bool verify_handshake_tag(
    const Key &key, const Nonce &nonce,
    const std::array<uint8_t, HANDSHAKE_CIPHERTEXT_SIZE> &ciphertext) {

  std::array<uint8_t, HANDSHAKE_TAG_SIZE> decrypted;

  if (crypto_secretbox_open_easy(decrypted.data(), ciphertext.data(),
                                 ciphertext.size(), nonce.data(),
                                 key.data()) != 0) {
    return false; // authentication failed (tampered or wrong key)
  }

  // Compare decrypted value with expected tag
  return std::memcmp(decrypted.data(), HANDSHAKE_TAG_LITERAL,
                     HANDSHAKE_TAG_SIZE) == 0;
}

} // namespace crypto
