#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include <gtest/gtest.h>

#include "api.pb.h"

#include <cstdint>
#include <string>

using esphome::api::MessageId;
using esphome::api::MessageRegistry;

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

// Round-trip every id in the generated enum table through the registry, in both
// directions, using only the public (reflection-free) API.
TEST(MessageRegistry, GeneratedTableRoundTrips) {
    std::size_t checked = 0;
    for (std::uint32_t id = 1; id <= esphome::api::message_id_max; ++id) {
        auto msg = esphome::api::MessageRegistry::create(id);
        if (msg == nullptr) {
            EXPECT_FALSE(esphome::api::MessageRegistry::contains(id)) << id;
            continue;
        }
        ++checked;
        EXPECT_TRUE(esphome::api::MessageRegistry::contains(id)) << id;

        // encode direction: message -> id
        EXPECT_EQ(MessageRegistry::id_of(*msg), id) << msg->message_name();

        // generated enum name matches the proto message name
        EXPECT_EQ(std::string(esphome::api::to_string(static_cast<MessageId>(id))),
                  std::string(msg->message_name()));
    }

    EXPECT_EQ(checked, esphome::api::MessageRegistry::size());
    EXPECT_EQ(checked, esphome::api::message_id_count);
}

TEST(MessageRegistry, UnknownIdReturnsNull) {
    EXPECT_FALSE(esphome::api::MessageRegistry::contains(0));
    EXPECT_FALSE(esphome::api::MessageRegistry::contains(99999));
    EXPECT_EQ(esphome::api::MessageRegistry::create(99999U), nullptr);
}

TEST(MessageRegistry, RoundTripThroughEnum) {
    const auto hello = esphome::api::MessageRegistry::create(MessageId::HelloRequest);
    ASSERT_NE(hello, nullptr);
    EXPECT_EQ(MessageRegistry::id_of(*hello), static_cast<std::uint32_t>(MessageId::HelloRequest));
    EXPECT_EQ(std::string(hello->message_name()), "HelloRequest");
}
