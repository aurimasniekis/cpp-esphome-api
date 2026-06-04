#pragma once

// ---------------------------------------------------------------------------
// Native per-OS CSPRNG. Prototype only; the platform implementation lives in
// random.cpp (compiled with relaxed warnings).
// ---------------------------------------------------------------------------

#include <cstddef>

namespace esphome::api::noise::detail {

/// Fill `[buf, buf+len)` with cryptographically secure random bytes from the OS
/// CSPRNG. Throws esphome::api::EncryptionError if the OS RNG fails (fail-closed).
void secure_random(void* buf, std::size_t len);

}  // namespace esphome::api::noise::detail
