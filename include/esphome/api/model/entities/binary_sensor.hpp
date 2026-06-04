#pragma once

/// @file
/// @brief Typed BinarySensor entity (info + state).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesBinarySensorResponse;
class BinarySensorStateResponse;
}  // namespace proto

/// Static description of a binary sensor entity.
struct BinarySensorInfo : EntityInfo {
    std::string device_class;
    bool is_status_binary_sensor = false;
};

/// A binary sensor's reported value.
struct BinarySensorState {
    std::uint32_t key = 0;
    bool state = false;
    /// True when the binary sensor currently has no valid value.
    bool missing_state = false;
};

BinarySensorInfo parse_binary_sensor_info(const proto::ListEntitiesBinarySensorResponse& msg);
BinarySensorState parse_binary_sensor_state(const proto::BinarySensorStateResponse& msg);

}  // namespace esphome::api
