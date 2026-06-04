#pragma once

// ---------------------------------------------------------------------------
// X25519 (Curve25519 ECDH) — vendored, public-domain reference.
//
// Source: TweetNaCl (tweetnacl.c) crypto_scalarmult / crypto_scalarmult_base,
//         https://tweetnacl.cr.yp.to/, version 20140427. Placed in the public
//         domain by Daniel J. Bernstein, Bernard van Gastel, Wesley Janssen,
//         Tanja Lange, Peter Schwabe and Sjaak Smetsers. Constant-time.
//
// Prototypes only; the implementation lives in x25519.cpp, compiled with
// relaxed warnings.
// ---------------------------------------------------------------------------

#include <cstdint>

namespace esphome::api::noise::detail {

/// Curve25519 scalar multiplication: q = scalar * point. All buffers are 32
/// bytes. Returns 0 on success (matching the upstream signature).
int x25519_scalarmult(std::uint8_t q[32],
                      const std::uint8_t scalar[32],
                      const std::uint8_t point[32]);

/// Curve25519 scalar multiplication against the base point (q = scalar * 9).
int x25519_scalarmult_base(std::uint8_t q[32], const std::uint8_t scalar[32]);

}  // namespace esphome::api::noise::detail
