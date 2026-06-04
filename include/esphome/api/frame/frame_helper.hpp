#pragma once

/// @file
/// @brief Abstract frame codec: turns a byte stream into ESPHome message frames
///        and back, independent of the underlying transport/socket.

#include <esphome/api/bytes.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace esphome::api {

/// Result of attempting to pull a frame from the reassembly buffer.
enum class FrameStatus {
    Ok,        ///< A complete frame is available in the out-parameters.
    NeedMore,  ///< The buffer holds only a partial frame; feed more bytes.
};

/// Stateful codec sitting between the transport and the connection.
///
/// A FrameHelper owns an internal reassembly buffer. The connection feeds it raw
/// bytes as they arrive (`feed`) and repeatedly calls `next` to drain whole
/// frames. Outgoing messages are serialized with `encode`.
///
/// Implementations: PlaintextFrameHelper (no encryption) and, when built with
/// Noise support, NoiseFrameHelper.
class FrameHelper {
public:
    FrameHelper() = default;
    virtual ~FrameHelper() = default;

    FrameHelper(const FrameHelper&) = delete;
    FrameHelper& operator=(const FrameHelper&) = delete;
    FrameHelper(FrameHelper&&) = delete;
    FrameHelper& operator=(FrameHelper&&) = delete;

    /// Serialize a message (numeric type id + protobuf payload) to wire bytes,
    /// appending them to `out`.
    virtual void encode(std::uint32_t msg_type, ByteView payload, ByteBuffer& out) = 0;

    /// Append freshly received bytes to the internal reassembly buffer.
    virtual void feed(ByteView data) = 0;

    /// Try to extract the next complete frame.
    ///
    /// On FrameStatus::Ok, `out_type` is the message type id and `out_payload`
    /// is a view of the protobuf payload that stays valid only until the next
    /// call to `feed` or `next`. On ::NeedMore nothing was consumed.
    ///
    /// Throws ProtocolError on a malformed or oversized frame.
    virtual FrameStatus next(std::uint32_t& out_type, ByteView& out_payload) = 0;

    // --- Handshake hooks (Noise overrides; plaintext is a no-op) -------------

    using HandshakeReady = std::function<void()>;
    using HandshakeError = std::function<void(const std::string& what)>;

    /// True if the transport requires a handshake before application messages
    /// (and before the connection should send HelloRequest).
    [[nodiscard]] virtual bool needs_handshake() const {
        return false;
    }

    /// Bytes to transmit immediately after the TCP connection is established
    /// (the client's first handshake flight). Empty for plaintext.
    virtual ByteBuffer connect_bytes() {
        return {};
    }

    /// Register callbacks fired when the handshake completes or fails. `next`
    /// consumes handshake frames internally and invokes these.
    // By value (not const ref) so overrides can move the callbacks into members;
    // this base no-op simply ignores them.
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    virtual void set_handshake_handlers(HandshakeReady /*on_ready*/, HandshakeError /*on_error*/) {}
};

/// Hard upper bound on a single frame's payload, guarding against a hostile or
/// corrupt length prefix (16 MiB — comfortably above camera/BLE batches).
inline constexpr std::size_t max_frame_payload = std::size_t{16} * 1024 * 1024;

}  // namespace esphome::api
