#pragma once

/// @file
/// @brief Noise_NNpsk0_25519_ChaChaPoly_SHA256 handshake (initiator + responder).

#include <esphome/api/bytes.hpp>
#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/crypto/primitives.hpp>
#include <esphome/api/crypto/symmetric_state.hpp>

namespace esphome::api::noise {

/// Noise protocol name for the ESPHome native API.
inline constexpr auto protocol_name = "Noise_NNpsk0_25519_ChaChaPoly_SHA256";

/// Drives the two-message NNpsk0 handshake. The client is the Initiator; the
/// Responder role exists for tests and the loopback emulator.
///
/// Token sequence: `-> psk, e` then `<- e, ee`. Under PSK, the `e` token mixes
/// the ephemeral public key into BOTH the hash and the key.
class NoiseHandshake {
public:
    enum class Role { Initiator, Responder };

    /// Set up with the role, the 32-byte PSK and the prologue.
    void initialize(Role role, const SymmetricKey& psk, ByteView prologue);

    /// Write the next handshake message (Initiator: message 1; Responder: 2).
    ByteBuffer write_message(ByteView payload = ByteView{});

    /// Read the next handshake message into `out_payload`. Returns false on an
    /// authentication failure (e.g. wrong PSK) or a too-short message.
    bool read_message(ByteView message, ByteBuffer& out_payload);

    [[nodiscard]] bool is_complete() const noexcept {
        return complete_;
    }

    /// After completion, derive the transport cipher states for this role
    /// (`send` is what this side encrypts with, `recv` what it decrypts with).
    void split(CipherState& send, CipherState& recv);

    [[nodiscard]] const Hash& handshake_hash() const noexcept {
        return symmetric_.handshake_hash();
    }

    /// Test hook: force a fixed ephemeral private key (for known-answer vectors).
    void set_ephemeral_for_testing(const PrivateKey& private_key);

private:
    void ensure_ephemeral();

    Role role_ = Role::Initiator;
    SymmetricKey psk_{};
    SymmetricState symmetric_;
    PrivateKey ephemeral_private_{};
    PublicKey ephemeral_public_{};
    PublicKey remote_ephemeral_{};
    bool have_ephemeral_ = false;
    bool complete_ = false;
};

}  // namespace esphome::api::noise
