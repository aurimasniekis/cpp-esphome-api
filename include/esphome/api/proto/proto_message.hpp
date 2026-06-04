#pragma once

/// @file
/// @brief Base class for every generated ESPHome API message — the lightweight
///        replacement for `google::protobuf::Message`.
///
/// Generated message classes (see the build-time `protogen` output
/// `api_messages.hpp`) derive from ProtoMessage and implement the wire
/// primitives. The compat wrappers (`ByteSizeLong`, `SerializeToArray`,
/// `ParseFromArray`, `SerializeAsString`) keep the few protobuf-shaped call
/// sites in connection.cpp / loopback_server.hpp unchanged.

#include <esphome/api/proto/wire.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace esphome::api {

/// Base class every generated message derives from.
class ProtoMessage {
public:
    ProtoMessage() = default;
    ProtoMessage(const ProtoMessage&) = default;
    ProtoMessage(ProtoMessage&&) = default;
    ProtoMessage& operator=(const ProtoMessage&) = default;
    ProtoMessage& operator=(ProtoMessage&&) = default;
    virtual ~ProtoMessage() = default;

    /// Serialize this message's fields to `writer` (ascending field number).
    virtual void encode(ProtoWriter& writer) const = 0;

    /// Decode wire bytes into this message. Returns false on truncation /
    /// malformed input. Unknown fields are skipped (forward-compat).
    virtual bool decode(ProtoReader& reader) = 0;

    /// Exact number of bytes `encode()` will write.
    [[nodiscard]] virtual std::size_t calculate_size() const = 0;

    /// The protocol message id (the proto `(id)` option), or 0 if the message
    /// carries none.
    [[nodiscard]] virtual std::uint32_t message_id() const = 0;

    /// The proto message name (e.g. "ListEntitiesLightResponse").
    [[nodiscard]] virtual const char* message_name() const = 0;

    /// A deep copy of this message as a new owning pointer.
    [[nodiscard]] virtual std::unique_ptr<ProtoMessage> clone() const = 0;

    // --- protobuf-compatible wrappers (non-virtual) ------------------------

    /// Alias for calculate_size() — matches protobuf's API.
    [[nodiscard]] std::size_t ByteSizeLong() const {
        return calculate_size();
    }

    /// Serialize into a caller-provided buffer of exactly `size` bytes. Returns
    /// false if the message does not encode to exactly `size` bytes.
    bool SerializeToArray(void* data, const int size) const {
        if (size < 0)
            return false;
        std::string buf;
        buf.reserve(static_cast<std::size_t>(size));
        ProtoWriter writer(buf);
        encode(writer);
        if (buf.size() != static_cast<std::size_t>(size))
            return false;
        if (!buf.empty())
            std::memcpy(data, buf.data(), buf.size());
        return true;
    }

    /// Serialize to a freshly allocated string.
    [[nodiscard]] std::string SerializeAsString() const {
        std::string buf;
        buf.reserve(calculate_size());
        ProtoWriter writer(buf);
        encode(writer);
        return buf;
    }

    /// Parse wire bytes into this message. Returns false on malformed input.
    bool ParseFromArray(const void* data, const int size) {
        if (size < 0)
            return false;
        ProtoReader reader(data, static_cast<std::size_t>(size));
        return decode(reader);
    }
};

/// Intermediate base for `ListEntities*Response` messages (proto
/// `(base_class) = "InfoResponseProtoMessage"`). Exposes the shared entity
/// fields as virtuals so EntityStore can read them without reflection.
class InfoResponseBase : public ProtoMessage {
public:
    [[nodiscard]] virtual std::uint32_t key() const = 0;
    [[nodiscard]] virtual const std::string& name() const = 0;
    [[nodiscard]] virtual const std::string& object_id() const = 0;
};

/// Intermediate base for `*StateResponse` messages (proto
/// `(base_class) = "StateResponseProtoMessage"`). Exposes the entity key as a
/// virtual so EntityStore can read it without reflection.
class StateResponseBase : public ProtoMessage {
public:
    [[nodiscard]] virtual std::uint32_t key() const = 0;
};

}  // namespace esphome::api
