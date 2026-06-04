#pragma once

/// @file
/// @brief Typed TextSensor entity (info + state).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesTextSensorResponse;
class TextSensorStateResponse;
}  // namespace proto

/// Static description of a text sensor entity.
struct TextSensorInfo : EntityInfo {
    std::string device_class;
};

/// A text sensor's reported value.
struct TextSensorState {
    std::uint32_t key = 0;
    std::string state;
    /// True when the text sensor currently has no valid value.
    bool missing_state = false;
};

TextSensorInfo parse_text_sensor_info(const proto::ListEntitiesTextSensorResponse& msg);
TextSensorState parse_text_sensor_state(const proto::TextSensorStateResponse& msg);

}  // namespace esphome::api
