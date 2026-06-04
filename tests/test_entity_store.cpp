#include <esphome/api/model/entity_store.hpp>

#include <gtest/gtest.h>

#include "api.pb.h"

#include <cstdint>
#include <string>

using esphome::api::EntityStore;
using esphome::api::EntityType;
namespace proto = esphome::api::proto;

TEST(EntityStore, IngestsInfoAndState) {
    EntityStore store;

    proto::ListEntitiesSensorResponse info;
    info.set_key(42);
    info.set_object_id("living_room_temp");
    info.set_name("Living Room Temperature");
    info.set_unit_of_measurement("°C");
    info.set_accuracy_decimals(1);
    EXPECT_TRUE(store.ingest(info));

    ASSERT_EQ(store.size(), 1U);
    const auto* e = store.find(42);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->type, EntityType::Sensor);
    EXPECT_EQ(e->object_id, "living_room_temp");

    auto si = store.sensor_info(42);
    ASSERT_TRUE(si.has_value());
    EXPECT_EQ(si->name, "Living Room Temperature");
    EXPECT_EQ(si->unit_of_measurement, "°C");
    EXPECT_EQ(si->accuracy_decimals, 1);

    // State.
    proto::SensorStateResponse state;
    state.set_key(42);
    state.set_state(21.5F);
    EXPECT_TRUE(store.ingest(state));

    auto ss = store.sensor_state(42);
    ASSERT_TRUE(ss.has_value());
    EXPECT_FLOAT_EQ(ss->state, 21.5F);
    EXPECT_FALSE(ss->missing_state);
}

TEST(EntityStore, StateCallbackFires) {
    EntityStore store;
    proto::ListEntitiesBinarySensorResponse info;
    info.set_key(7);
    info.set_object_id("motion");
    store.ingest(info);

    int calls = 0;
    auto seen_type = EntityType::Unknown;
    store.on_state([&](const EntityType type, const std::uint32_t key) {
        ++calls;
        seen_type = type;
        EXPECT_EQ(key, 7U);
    });

    proto::BinarySensorStateResponse state;
    state.set_key(7);
    state.set_state(true);
    store.ingest(state);

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(seen_type, EntityType::BinarySensor);
    auto bs = store.binary_sensor_state(7);
    ASSERT_TRUE(bs.has_value());
    EXPECT_TRUE(bs->state);
}

TEST(EntityStore, LookupByObjectIdAndTypeMismatch) {
    EntityStore store;
    proto::ListEntitiesSwitchResponse sw;
    sw.set_key(5);
    sw.set_object_id("relay");
    store.ingest(sw);

    EXPECT_EQ(store.find_by_object_id("relay")->key, 5U);
    EXPECT_EQ(store.find_by_object_id("nope"), nullptr);

    // Wrong-typed accessor returns nullopt.
    EXPECT_FALSE(store.sensor_info(5).has_value());
    EXPECT_TRUE(store.switch_info(5).has_value());
}

TEST(EntityStore, IgnoresNonEntityMessages) {
    EntityStore store;
    const proto::PingRequest ping;
    EXPECT_FALSE(store.ingest(ping));
    EXPECT_EQ(store.size(), 0U);
}

TEST(EntityStore, MultipleDomains) {
    EntityStore store;
    proto::ListEntitiesLightResponse light;
    light.set_key(1);
    light.set_object_id("lamp");
    store.ingest(light);
    proto::ListEntitiesClimateResponse climate;
    climate.set_key(2);
    climate.set_object_id("thermostat");
    store.ingest(climate);

    EXPECT_EQ(store.size(), 2U);
    EXPECT_EQ(store.find(1)->type, EntityType::Light);
    EXPECT_EQ(store.find(2)->type, EntityType::Climate);
    EXPECT_TRUE(store.light_info(1).has_value());
    EXPECT_TRUE(store.climate_info(2).has_value());
}
