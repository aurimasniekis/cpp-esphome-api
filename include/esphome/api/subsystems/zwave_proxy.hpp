#pragma once

/// @file
/// @brief Z-Wave proxy subsystem.

#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/subsystem.hpp>

#include <functional>
#include <string>

namespace esphome::api {

/// A raw Z-Wave frame exchanged with the device (api: ZWaveProxyFrame).
struct ZWaveProxyFrame {
    std::string data;
};

/// Bridges raw Z-Wave frames between a client and the device's Z-Wave radio.
class ZWaveProxy : public Subsystem {
public:
    using FrameHandler = std::function<void(const ZWaveProxyFrame&)>;

    explicit ZWaveProxy(Client& client) : Subsystem(client) {}

    /// Register a callback fired for each incoming ZWaveProxyFrame.
    void on_frame(FrameHandler handler);

    /// Send a raw Z-Wave frame to the device.
    void send_frame(const std::string& data) const;

    /// Issue a proxy control request (subscribe, unsubscribe, home id change).
    /// `data` carries the optional request payload.
    void request(ZWaveProxyRequestType type, const std::string& data = {}) const;

private:
    FrameHandler frame_handler_;
};

}  // namespace esphome::api
