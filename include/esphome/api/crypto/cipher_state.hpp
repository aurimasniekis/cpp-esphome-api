#pragma once

/// @file
/// @brief Noise CipherState: a keyed AEAD with an incrementing nonce.

#include <esphome/api/bytes.hpp>
#include <esphome/api/crypto/primitives.hpp>

#include <cstdint>

namespace esphome::api::noise {

/// Wraps a symmetric key plus a monotonically increasing nonce, per the Noise
/// CipherState object. Without a key, encrypt/decrypt are pass-through.
class CipherState {
public:
    /// Set the key and reset the nonce to zero.
    void initialize_key(const SymmetricKey& key);

    [[nodiscard]] bool has_key() const noexcept {
        return has_key_;
    }
    [[nodiscard]] std::uint64_t nonce() const noexcept {
        return nonce_;
    }

    /// AEAD-encrypt `plaintext` with associated data `ad`, advancing the nonce.
    /// Returns a copy of `plaintext` if no key is set.
    ByteBuffer encrypt_with_ad(ByteView ad, ByteView plaintext);

    /// AEAD-decrypt `ciphertext`, advancing the nonce. Throws EncryptionError on
    /// authentication failure. Returns a copy of `ciphertext` if no key is set.
    ByteBuffer decrypt_with_ad(ByteView ad, ByteView ciphertext);

private:
    SymmetricKey key_{};
    std::uint64_t nonce_ = 0;
    bool has_key_ = false;
};

}  // namespace esphome::api::noise
