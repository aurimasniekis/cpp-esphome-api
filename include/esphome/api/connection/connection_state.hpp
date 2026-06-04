#pragma once

/// @file
/// @brief Connection state-machine states.

namespace esphome::api {

/// Lifecycle of a Connection. Transitions are strictly monotonic per attempt:
/// Disconnected → Connecting → HelloSent → Connected → Closing → Closed, with
/// Failed reachable from any active state on error.
enum class ConnectionState {
    Disconnected,  ///< Idle, never connected or fully torn down.
    Connecting,    ///< TCP connect (and, for Noise, handshake) in progress.
    HelloSent,     ///< HelloRequest sent; awaiting HelloResponse.
    Connected,     ///< Handshake complete; messages flow freely.
    Closing,       ///< Graceful DisconnectRequest in flight.
    Closed,        ///< Cleanly closed.
    Failed,        ///< Terminated by an error.
};

/// Human-readable name of a connection state.
inline const char* to_string(const ConnectionState state) {
    switch (state) {
    case ConnectionState::Disconnected:
        return "Disconnected";
    case ConnectionState::Connecting:
        return "Connecting";
    case ConnectionState::HelloSent:
        return "HelloSent";
    case ConnectionState::Connected:
        return "Connected";
    case ConnectionState::Closing:
        return "Closing";
    case ConnectionState::Closed:
        return "Closed";
    case ConnectionState::Failed:
        return "Failed";
    }
    return "Unknown";
}

}  // namespace esphome::api
