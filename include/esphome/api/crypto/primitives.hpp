#pragma once

/// @file
/// @brief Noise cryptographic primitives (SHA-256, HMAC, HKDF, X25519, ChaCha20-
///        Poly1305, base64). The implementation (src/crypto/primitives.cpp)
///        wraps vendored, public-domain reference code under src/crypto/detail/
///        plus the OS CSPRNG. Built only when ESPHOME_API_WITH_NOISE is on.

#include <esphome/api/bytes.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace esphome::api::noise {

inline constexpr std::size_t hash_len = 32;  ///< SHA-256 digest / key size.
inline constexpr std::size_t key_len = 32;   ///< Symmetric / DH key size.

using Hash = std::array<std::uint8_t, hash_len>;
using SymmetricKey = std::array<std::uint8_t, key_len>;
using PublicKey = std::array<std::uint8_t, key_len>;
using PrivateKey = std::array<std::uint8_t, key_len>;

/// SHA-256 of `data`.
Hash sha256(ByteView data);

/// HMAC-SHA-256 with `key` over `data`.
Hash hmac_sha256(ByteView key, ByteView data);

/// Noise HKDF: derive `num_outputs` (1..3) 32-byte values from `chaining_key`
/// and `input_key_material`. Only the first `num_outputs` of o1/o2/o3 are valid.
void hkdf(const Hash& chaining_key,
          ByteView input_key_material,
          int num_outputs,
          Hash& o1,
          Hash& o2,
          Hash& o3);

/// X25519 public key for a private scalar.
PublicKey x25519_base(const PrivateKey& private_key);

/// X25519 shared secret. Throws EncryptionError on an all-zero (degenerate) result.
SymmetricKey x25519(const PrivateKey& private_key, const PublicKey& peer_public);

/// Generate a fresh X25519 keypair from the system CSPRNG.
void generate_keypair(PrivateKey& private_key, PublicKey& public_key);

/// AEAD ChaCha20-Poly1305 (IETF) encryption. `nonce` is the Noise message
/// counter (encoded as 4 zero bytes + 8-byte little-endian). Returns
/// ciphertext||tag.
ByteBuffer
aead_encrypt(const SymmetricKey& key, std::uint64_t nonce, ByteView ad, ByteView plaintext);

/// AEAD ChaCha20-Poly1305 (IETF) decryption. Returns false on auth failure.
bool aead_decrypt(const SymmetricKey& key,
                  std::uint64_t nonce,
                  ByteView ad,
                  ByteView ciphertext,
                  ByteBuffer& out_plaintext);

/// Decode standard base64. Returns false on malformed input.
bool base64_decode(const std::string& text, ByteBuffer& out);

}  // namespace esphome::api::noise
