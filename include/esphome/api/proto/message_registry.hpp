#pragma once

/// @file
/// @brief Bidirectional registry mapping ESPHome message ids to message types.

#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace esphome::api {

/// Maps the protocol's numeric message ids (the proto `(id)` option) to and
/// from concrete message types.
///
/// Backed by a generated id<->type table (see api_registry.cpp) — no runtime
/// reflection. The encode direction (`id_of`) reads the message's own id; the
/// decode direction (`create`) is a generated switch.
class MessageRegistry {
public:
    /// The process-wide registry (constructed on first use).
    static const MessageRegistry& instance();

    /// Create a default-constructed message for `id`, or nullptr if no message
    /// type carries that id.
    [[nodiscard]] static std::unique_ptr<ProtoMessage> create(std::uint32_t id);
    [[nodiscard]] static std::unique_ptr<ProtoMessage> create(MessageId id);

    /// Whether a message type with `id` exists.
    [[nodiscard]] static bool contains(std::uint32_t id);

    /// Number of registered message types.
    [[nodiscard]] static std::size_t size() noexcept;

    /// The message id of a concrete message instance (0 if it carries none).
    [[nodiscard]] static std::uint32_t id_of(const ProtoMessage& msg);

    MessageRegistry(const MessageRegistry&) = delete;
    MessageRegistry& operator=(const MessageRegistry&) = delete;

private:
    MessageRegistry() = default;
};

}  // namespace esphome::api
