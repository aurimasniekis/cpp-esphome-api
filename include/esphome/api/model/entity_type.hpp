#pragma once

/// @file
/// @brief Enumeration of ESPHome entity domains and name conversion helpers.

#include <string>

namespace esphome::api {

/// One per ESPHome entity domain (the `<X>` in ListEntities<X>Response).
enum class EntityType {
    Unknown,
    BinarySensor,
    Sensor,
    TextSensor,
    Switch,
    Light,
    Cover,
    Fan,
    Climate,
    Number,
    Select,
    Text,
    Button,
    Lock,
    MediaPlayer,
    Camera,
    Siren,
    AlarmControlPanel,
    Date,
    Time,
    DateTime,
    Valve,
    Event,
    Update,
    WaterHeater,
    Infrared,
    RadioFrequency,
};

/// Domain name, e.g. EntityType::BinarySensor -> "BinarySensor".
const char* entity_type_name(EntityType type);

/// Parse the `<X>` token from a message name (e.g. "BinarySensor") into the
/// enum, or EntityType::Unknown if unrecognised.
EntityType entity_type_from_token(const std::string& token);

}  // namespace esphome::api
