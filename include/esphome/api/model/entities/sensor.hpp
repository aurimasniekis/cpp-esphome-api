#pragma once

/// @file
/// @brief Typed Sensor entity (info + state).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesSensorResponse;
class SensorStateResponse;
}  // namespace proto

/// Static description of a sensor entity.
struct SensorInfo : EntityInfo {
    std::string unit_of_measurement;
    std::int32_t accuracy_decimals = 0;
    bool force_update = false;
    std::string device_class;
    SensorStateClass state_class = SensorStateClass::None;
};

/// A sensor's reported value.
struct SensorState {
    std::uint32_t key = 0;
    float state = 0.0F;
    /// True when the sensor currently has no valid value.
    bool missing_state = false;
};

SensorInfo parse_sensor_info(const proto::ListEntitiesSensorResponse& msg);
SensorState parse_sensor_state(const proto::SensorStateResponse& msg);

}  // namespace esphome::api
