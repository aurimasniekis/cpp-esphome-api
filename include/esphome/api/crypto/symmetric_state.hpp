#pragma once

/// @file
/// @brief Noise SymmetricState: chaining key + transcript hash + CipherState.

#include <esphome/api/bytes.hpp>
#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/crypto/primitives.hpp>

#include <string>
#include <utility>

namespace esphome::api::noise {

/// Implements the Noise SymmetricState object: it accumulates the handshake
/// transcript hash `h`, derives keys through the chaining key `ck`, and owns the
/// handshake CipherState.
class SymmetricState {
public:
    /// Initialize from the protocol name (e.g. "Noise_NNpsk0_25519_ChaChaPoly_SHA256").
    void initialize(const std::string& protocol_name);

    /// MixKey(input): re-derive ck and the CipherState key.
    void mix_key(ByteView input);

    /// MixHash(data): fold `data` into the transcript hash.
    void mix_hash(ByteView data);

    /// MixKeyAndHash(input): the psk-mixing variant (HKDF with 3 outputs).
    void mix_key_and_hash(ByteView input);

    /// EncryptAndHash(plaintext): encrypt (if keyed), fold ciphertext into `h`.
    ByteBuffer encrypt_and_hash(ByteView plaintext);

    /// DecryptAndHash(ciphertext): decrypt (if keyed), fold ciphertext into `h`.
    /// Throws EncryptionError on authentication failure.
    ByteBuffer decrypt_and_hash(ByteView ciphertext);

    /// Split(): derive the two transport CipherStates (initiator: first=send).
    void split(CipherState& first, CipherState& second) const;

    [[nodiscard]] const Hash& handshake_hash() const noexcept {
        return h_;
    }

private:
    Hash ck_{};
    Hash h_{};
    CipherState cipher_;
};

}  // namespace esphome::api::noise
