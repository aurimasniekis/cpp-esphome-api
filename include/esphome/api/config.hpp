#pragma once

/// @file
/// @brief Compile-time configuration flags for esphome-api-client.

namespace esphome::api {

/// True when the library was built with Noise (encrypted-transport) support.
/// Controlled by the CMake option `ESPHOME_API_WITH_NOISE`, which defines the
/// `ESPHOME_API_HAS_NOISE` macro on the library target.
#if defined(ESPHOME_API_HAS_NOISE)
inline constexpr bool noise_supported = true;
#else
inline constexpr bool noise_supported = false;
#endif

/// Default TCP port of the ESPHome native API.
inline constexpr unsigned default_port = 6053;

}  // namespace esphome::api
