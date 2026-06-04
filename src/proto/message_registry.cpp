#include <esphome/api/proto/message_registry.hpp>

#include "api.pb.h"
#include "api_options.pb.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace esphome::api {

MessageRegistry::MessageRegistry() {
    const google::protobuf::FileDescriptor* file = proto::HelloRequest::descriptor()->file();
    by_id_.reserve(static_cast<std::size_t>(file->message_type_count()));
    for (int i = 0; i < file->message_type_count(); ++i) {
        const google::protobuf::Descriptor* d = file->message_type(i);
        if (const std::uint32_t id = d->options().GetExtension(proto::id); id != 0) {
            by_id_.emplace(id, d);
        }
    }
}

const MessageRegistry& MessageRegistry::instance() {
    static const MessageRegistry registry;
    return registry;
}

const google::protobuf::Descriptor* MessageRegistry::descriptor(const std::uint32_t id) const {
    const auto it = by_id_.find(id);
    return it == by_id_.end() ? nullptr : it->second;
}

bool MessageRegistry::contains(const std::uint32_t id) const {
    return by_id_.find(id) != by_id_.end();
}

std::size_t MessageRegistry::size() const noexcept {
    return by_id_.size();
}

std::unique_ptr<ProtoMessage> MessageRegistry::create(const std::uint32_t id) const {
    const google::protobuf::Descriptor* d = descriptor(id);
    if (d == nullptr) {
        return nullptr;
    }
    const google::protobuf::Message* prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(d);
    if (prototype == nullptr) {
        return nullptr;
    }
    return std::unique_ptr<ProtoMessage>(prototype->New());
}

std::unique_ptr<ProtoMessage> MessageRegistry::create(MessageId id) const {
    return create(static_cast<std::uint32_t>(id));
}

std::uint32_t MessageRegistry::id_of(const ProtoMessage& msg) {
    return msg.GetDescriptor()->options().GetExtension(proto::id);
}

}  // namespace esphome::api
