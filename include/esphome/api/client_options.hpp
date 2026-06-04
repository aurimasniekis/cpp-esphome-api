#pragma once

/// @file
/// @brief Top-level connection parameters for Client / SyncClient.

#include <esphome/api/config.hpp>
#include <esphome/api/connection/connection_options.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace esphome::api {

/// Everything needed to reach and authenticate with one ESPHome device.
struct ClientOptions {
    /// Device hostname or IP.
    std::string host;

    /// TCP port (default 6053).
    std::uint16_t port = static_cast<std::uint16_t>(default_port);

    /// Handshake / keepalive tunables (client_info, Noise PSK, timeouts, ...).
    ConnectionOptions connection;

    /// When true (default), SyncClient::connect() automatically enumerates the
    /// device's entities and subscribes to state updates, so entities() is
    /// populated and live as soon as connect() returns.
    bool subscribe_on_connect = true;

    ClientOptions() = default;
    ClientOptions(std::string h, const std::uint16_t p = static_cast<std::uint16_t>(default_port))
        : host(std::move(h)), port(p) {}
};

}  // namespace esphome::api
