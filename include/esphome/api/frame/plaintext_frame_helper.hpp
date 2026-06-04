#pragma once

/// @file
/// @brief Plaintext (unencrypted) ESPHome frame codec.

#include <esphome/api/bytes.hpp>
#include <esphome/api/frame/frame_helper.hpp>

#include <cstddef>
#include <cstdint>

namespace esphome::api {

/// Codec for the plaintext native-API framing:
///
///     [0x00][varint payload_length][varint msg_type][protobuf payload]
///
/// The leading `0x00` indicator distinguishes plaintext from Noise (`0x01`).
class PlaintextFrameHelper final : public FrameHelper {
public:
    /// Indicator byte that prefixes every plaintext frame.
    static constexpr std::uint8_t indicator = 0x00;

    void encode(std::uint32_t msg_type, ByteView payload, ByteBuffer& out) override;
    void feed(ByteView data) override;
    FrameStatus next(std::uint32_t& out_type, ByteView& out_payload) override;

private:
    /// Drop the bytes consumed by the previous successful `next`.
    void compact();

    ByteBuffer buffer_;
    std::size_t consumed_ = 0;
};

}  // namespace esphome::api
