#pragma once

/// @file
/// @brief Bidirectional registry mapping ESPHome message ids to protobuf types.

#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace esphome::api {

/// Maps the protocol's numeric message ids (the proto `(id)` option) to and
/// from concrete protobuf message types.
///
/// Built once, lazily, by reflecting over the generated api.proto descriptors —
/// no codegen table to keep in sync. The encode direction (`id_of`) is a direct
/// O(1) descriptor-option read; the decode direction (`create`) is a hash-map
/// prototype lookup.
class MessageRegistry {
public:
    /// The process-wide registry (constructed on first use).
    static const MessageRegistry& instance();

    /// Create a default-constructed message for `id`, or nullptr if no message
    /// type carries that id.
    [[nodiscard]] std::unique_ptr<ProtoMessage> create(std::uint32_t id) const;
    [[nodiscard]] std::unique_ptr<ProtoMessage> create(MessageId id) const;

    /// Descriptor for `id`, or nullptr if unknown.
    [[nodiscard]] const google::protobuf::Descriptor* descriptor(std::uint32_t id) const;

    /// Whether a message type with `id` exists.
    [[nodiscard]] bool contains(std::uint32_t id) const;

    /// Number of registered message types.
    [[nodiscard]] std::size_t size() const noexcept;

    /// The message id of a concrete message instance (0 if it carries none).
    [[nodiscard]] static std::uint32_t id_of(const ProtoMessage& msg);

    MessageRegistry(const MessageRegistry&) = delete;
    MessageRegistry& operator=(const MessageRegistry&) = delete;

private:
    MessageRegistry();

    std::unordered_map<std::uint32_t, const google::protobuf::Descriptor*> by_id_;
};

}  // namespace esphome::api
