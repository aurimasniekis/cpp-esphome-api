#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include <gtest/gtest.h>

#include "api.pb.h"
#include "api_options.pb.h"

#include <cstdint>
#include <string>

#include <google/protobuf/descriptor.h>

using esphome::api::MessageId;
using esphome::api::MessageRegistry;

namespace {

const google::protobuf::FileDescriptor* api_file() {
    return esphome::api::proto::HelloRequest::descriptor()->file();
}

}  // namespace

TEST(MessageRegistry, NonEmptyAndMatchesGeneratedCount) {
    const auto& reg = MessageRegistry::instance();
    EXPECT_GT(reg.size(), 100U);
    EXPECT_EQ(reg.size(), esphome::api::message_id_count);
}

TEST(MessageRegistry, KnownControlMessageIds) {
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::HelloRequest), 1U);
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::HelloResponse), 2U);
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::DisconnectRequest), 5U);
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::DisconnectResponse), 6U);
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::PingRequest), 7U);
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::PingResponse), 8U);
    EXPECT_EQ(static_cast<std::uint32_t>(MessageId::DeviceInfoRequest), 9U);
}

// The generated enum/table must agree with runtime descriptor reflection, in
// both directions, for every message that carries an (id).
TEST(MessageRegistry, GeneratedTableEqualsReflection) {
    const auto& reg = MessageRegistry::instance();
    const auto* file = api_file();

    std::size_t reflected = 0;
    for (int i = 0; i < file->message_type_count(); ++i) {
        const google::protobuf::Descriptor* d = file->message_type(i);
        const std::uint32_t id = d->options().GetExtension(esphome::api::proto::id);
        if (id == 0) {
            continue;
        }
        ++reflected;

        // decode direction: id -> descriptor / fresh message
        EXPECT_EQ(reg.descriptor(id), d) << d->name();
        EXPECT_TRUE(reg.contains(id)) << d->name();

        auto msg = reg.create(id);
        ASSERT_NE(msg, nullptr) << d->name();
        EXPECT_EQ(msg->GetDescriptor(), d) << d->name();

        // encode direction: message -> id
        EXPECT_EQ(MessageRegistry::id_of(*msg), id) << d->name();

        // generated enum name matches the proto message name
        EXPECT_EQ(std::string(esphome::api::to_string(static_cast<MessageId>(id))),
                  std::string(d->name()));
    }

    EXPECT_EQ(reflected, reg.size());
    EXPECT_EQ(reflected, esphome::api::message_id_count);
}

TEST(MessageRegistry, UnknownIdReturnsNull) {
    const auto& reg = MessageRegistry::instance();
    EXPECT_FALSE(reg.contains(0));
    EXPECT_FALSE(reg.contains(99999));
    EXPECT_EQ(reg.create(99999U), nullptr);
    EXPECT_EQ(reg.descriptor(99999U), nullptr);
}

TEST(MessageRegistry, RoundTripThroughEnum) {
    const auto& reg = MessageRegistry::instance();
    const auto hello = reg.create(MessageId::HelloRequest);
    ASSERT_NE(hello, nullptr);
    EXPECT_EQ(MessageRegistry::id_of(*hello), static_cast<std::uint32_t>(MessageId::HelloRequest));
    EXPECT_EQ(hello->GetDescriptor(), esphome::api::proto::HelloRequest::descriptor());
}
