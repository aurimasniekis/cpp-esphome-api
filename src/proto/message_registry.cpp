#include <esphome/api/proto/message_registry.hpp>

#include "api.pb.h"

namespace esphome::api {

const MessageRegistry& MessageRegistry::instance() {
    static constexpr MessageRegistry registry;
    return registry;
}

std::unique_ptr<ProtoMessage> MessageRegistry::create(const std::uint32_t id) {
    return proto::create_message(id);
}

std::unique_ptr<ProtoMessage> MessageRegistry::create(const MessageId id) {
    return create(static_cast<std::uint32_t>(id));
}

bool MessageRegistry::contains(const std::uint32_t id) {
    return proto::registry_contains(id);
}

std::size_t MessageRegistry::size() noexcept {
    return proto::registry_size();
}

std::uint32_t MessageRegistry::id_of(const ProtoMessage& msg) {
    return msg.message_id();
}

}  // namespace esphome::api
