#pragma once

// ---------------------------------------------------------------------------
// SHA-256 (streaming) — vendored, public-domain reference.
//
// Source: Brad Conte's "crypto-algorithms" (sha256.c / sha256.h),
//         https://github.com/B-Con/crypto-algorithms, commit cfbde48 (no
//         versioned releases). Released into the public domain by its author.
//
// Only the prototypes live here so the strict-warning wrapper
// (src/crypto/primitives.cpp) can include this header safely. The
// implementation is in sha256.cpp, compiled with relaxed warnings.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>

namespace esphome::api::noise::detail {

inline constexpr std::size_t sha256_digest_len = 32;

/// Streaming SHA-256 state. Use init / update / final.
struct Sha256Ctx {
    std::uint8_t data[64];
    std::uint32_t datalen;
    std::uint64_t bitlen;
    std::uint32_t state[8];
};

void sha256_init(Sha256Ctx& ctx);
void sha256_update(Sha256Ctx& ctx, const std::uint8_t* data, std::size_t len);
/// Writes `sha256_digest_len` (32) bytes into `hash`.
void sha256_final(Sha256Ctx& ctx, std::uint8_t* hash);

/// One-shot convenience: SHA-256 of `[data, data+len)` into `hash` (32 bytes).
void sha256(const std::uint8_t* data, std::size_t len, std::uint8_t* hash);

}  // namespace esphome::api::noise::detail
