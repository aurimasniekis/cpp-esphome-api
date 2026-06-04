#pragma once

// ---------------------------------------------------------------------------
// ChaCha20-Poly1305 IETF AEAD (RFC 8439) — vendored, public-domain primitives.
//
// ChaCha20 is a from-scratch RFC 8439 reference implementation. Poly1305 is
// derived from Andrew Moon's "poly1305-donna" (poly1305-donna-32.h),
// https://github.com/floodyberry/poly1305-donna (commit e6ad6e0), released
// into the public domain (MIT0 / "no warranty, do whatever"). Its constant-time
// poly1305_verify is preserved for AEAD tag comparison.
//
// Prototypes only; the implementation lives in chacha20poly1305.cpp, compiled
// with relaxed warnings. The strict-warning wrapper (primitives.cpp) includes
// only this header.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>

namespace esphome::api::noise::detail {

inline constexpr std::size_t chacha20poly1305_key_len = 32;
inline constexpr std::size_t chacha20poly1305_nonce_len = 12;
inline constexpr std::size_t chacha20poly1305_tag_len = 16;

/// AEAD seal: encrypt `[plaintext, plaintext+pt_len)` into `ciphertext` (same
/// length) and write the 16-byte authentication tag into `tag`. `ciphertext`
/// may alias `plaintext`. `ad`/`plaintext` may be null when their length is 0.
void chacha20poly1305_encrypt(const std::uint8_t key[chacha20poly1305_key_len],
                              const std::uint8_t nonce[chacha20poly1305_nonce_len],
                              const std::uint8_t* ad,
                              std::size_t ad_len,
                              const std::uint8_t* plaintext,
                              std::size_t pt_len,
                              std::uint8_t* ciphertext,
                              std::uint8_t tag[chacha20poly1305_tag_len]);

/// AEAD open: verify `tag` over `ad`/`ciphertext` and, on success, decrypt
/// `[ciphertext, ciphertext+ct_len)` into `plaintext` (same length). The tag is
/// compared in constant time; returns false (and leaves `plaintext` unspecified)
/// on authentication failure.
bool chacha20poly1305_decrypt(const std::uint8_t key[chacha20poly1305_key_len],
                              const std::uint8_t nonce[chacha20poly1305_nonce_len],
                              const std::uint8_t* ad,
                              std::size_t ad_len,
                              const std::uint8_t* ciphertext,
                              std::size_t ct_len,
                              const std::uint8_t tag[chacha20poly1305_tag_len],
                              std::uint8_t* plaintext);

}  // namespace esphome::api::noise::detail
