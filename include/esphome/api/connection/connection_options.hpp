#pragma once

/// @file
/// @brief Tunables for a Connection's handshake and keepalive behaviour.

#include <chrono>
#include <string>

namespace esphome::api {

/// Knobs controlling a single Connection. Defaults mirror aioesphomeapi.
struct ConnectionOptions {
    /// Client identification reported to the device in HelloRequest.
    std::string client_info = "esphome-api-client";

    /// Optional pre-shared Noise key (base64, 32 bytes). Empty ⇒ plaintext.
    std::string noise_psk;

    /// Optional expected device name; if set and the device reports a different
    /// name the handshake fails (guards against connecting to the wrong host).
    std::string expected_name;

    /// Send a login (AuthenticationRequest) right after HelloRequest. The device
    /// treats the connection as authenticated only after this; without it the
    /// device logs "requested access without authentication". Default on.
    bool login = true;

    /// Optional plaintext API password (deprecated ESPHome auth). Sent in the
    /// AuthenticationRequest when `login` is true; empty means "no password".
    std::string password;

    /// Overall deadline for TCP connect + handshake + HelloResponse.
    std::chrono::milliseconds connect_timeout{30000};

    /// How often to send a keepalive PingRequest once connected.
    std::chrono::milliseconds keepalive_interval{20000};

    /// How long to wait for a PingResponse before declaring the link dead.
    std::chrono::milliseconds keepalive_timeout{90000};
};

}  // namespace esphome::api
