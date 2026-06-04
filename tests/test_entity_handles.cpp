// Object-oriented entity handle API over the real stack + loopback server.

#include <esphome/api/api.hpp>

#include <gtest/gtest.h>

#include "support/loopback_server.hpp"

#include <chrono>
#include <cstdint>
#include <string>

using esphome::api::ClientOptions;
using esphome::api::EntityType;
using esphome::api::SyncClient;
using esphome::api::testing::LoopbackServer;

namespace {
ClientOptions local_options(const std::uint16_t port) {
    ClientOptions opt;
    opt.host = "127.0.0.1";
    opt.port = port;
    opt.connection.connect_timeout = std::chrono::seconds(5);
    return opt;
}
}  // namespace

class EntityHandles : public ::testing::Test {
protected:
    void SetUp() override {
        server_.start();
        client_ = std::make_unique<SyncClient>(local_options(server_.port()));
        client_->connect();  // auto-enumerates entities + subscribes
    }
    void TearDown() override {
        if (client_) {
            client_->disconnect();
        }
    }
    LoopbackServer server_;
    std::unique_ptr<SyncClient> client_;
};

TEST_F(EntityHandles, AutoEnumeratedOnConnect) {
    // connect() listed entities automatically.
    EXPECT_GE(client_->store().size(), 3U);
    EXPECT_EQ(client_->entities().switches().size(), 1U);
    EXPECT_EQ(client_->entities().lights().size(), 1U);
    EXPECT_EQ(client_->entities().sensors().size(), 1U);
}

TEST_F(EntityHandles, TypedAccessorsAndLookup) {
    auto sensors = client_->entities().sensors();
    ASSERT_EQ(sensors.size(), 1U);
    auto s = sensors[0];
    EXPECT_EQ(s.name(), "Temperature");
    EXPECT_EQ(s.object_id(), "temperature");
    EXPECT_EQ(s.unit_of_measurement(),
              "\xc2\xb0"
              "C");
    EXPECT_FLOAT_EQ(s.value(), 21.5F);  // state arrived via subscribe
    EXPECT_TRUE(s.has_value());

    // Lookup by object_id and key.
    auto by_id = client_->entities().sensor("temperature");
    ASSERT_TRUE(by_id.has_value());
    EXPECT_EQ(by_id->key(), 12U);
    auto by_key = client_->entities().sensor(12U);
    ASSERT_TRUE(by_key.has_value());
    EXPECT_EQ(by_key->object_id(), "temperature");
    // Wrong-typed lookup returns nullopt.
    EXPECT_FALSE(client_->entities().light(12U).has_value());
}

TEST_F(EntityHandles, SwitchCommandRoundTrip) {
    auto sw = client_->entities().switch_("relay");
    ASSERT_TRUE(sw.has_value());
    EXPECT_EQ(sw->name(), "Relay");

    sw->turn_on();
    // Pump until the echoed state arrives.
    try {
        client_->pump_until([&] { return sw->is_on(); }, std::chrono::seconds(2));
    } catch (...) {}
    EXPECT_TRUE(sw->is_on());

    sw->turn_off();
    try {
        client_->pump_until([&] { return !sw->is_on(); }, std::chrono::seconds(2));
    } catch (...) {}
    EXPECT_FALSE(sw->is_on());
}

TEST_F(EntityHandles, AllEntitiesIterable) {
    std::size_t controllable = 0;
    for (const auto* e : client_->store().entities()) {
        if (e->type == EntityType::Switch || e->type == EntityType::Light) {
            ++controllable;
        }
    }
    EXPECT_EQ(controllable, 2U);
}
