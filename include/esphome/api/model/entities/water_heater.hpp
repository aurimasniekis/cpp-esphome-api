#pragma once

/// @file
/// @brief Typed WaterHeater entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesWaterHeaterResponse;
class WaterHeaterStateResponse;
class WaterHeaterCommandRequest;
}  // namespace proto

/// Static description of a water heater entity.
struct WaterHeaterInfo : EntityInfo {
    float min_temperature = 0.0F;
    float max_temperature = 0.0F;
    float target_temperature_step = 0.0F;
    std::vector<WaterHeaterMode> supported_modes;
    /// Bitmask of WaterHeaterFeature flags.
    std::uint32_t supported_features = 0;
    TemperatureUnit temperature_unit = TemperatureUnit::Celsius;
};

/// A water heater's reported state.
struct WaterHeaterState {
    std::uint32_t key = 0;
    float current_temperature = 0.0F;
    float target_temperature = 0.0F;
    WaterHeaterMode mode = WaterHeaterMode::Off;
    /// Bitmask of current state flags (bit 0 = away, bit 1 = on).
    std::uint32_t state = 0;
    float target_temperature_low = 0.0F;
    float target_temperature_high = 0.0F;
};

/// A command to mutate a water heater. The command signals which fields are set
/// via the `has_fields` bitmask (see WaterHeaterCommandHasField); here each set
/// field is represented by an engaged optional, mapped to the matching bit in
/// `to_message`.
struct WaterHeaterCommand {
    std::uint32_t key = 0;
    std::optional<WaterHeaterMode> mode;
    std::optional<float> target_temperature;
    std::optional<float> target_temperature_low;
    std::optional<float> target_temperature_high;
    /// On/off flag (bit 1 of the state bitmask).
    std::optional<bool> on_state;
    /// Away flag (bit 0 of the state bitmask).
    std::optional<bool> away_state;
};

WaterHeaterInfo parse_water_heater_info(const proto::ListEntitiesWaterHeaterResponse& msg);
WaterHeaterState parse_water_heater_state(const proto::WaterHeaterStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const WaterHeaterCommand& cmd);

}  // namespace esphome::api
