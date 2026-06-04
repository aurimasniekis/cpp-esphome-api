#pragma once

/// @file
/// @brief Typed Climate entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesClimateResponse;
class ClimateStateResponse;
class ClimateCommandRequest;
}  // namespace proto

/// Static description of a climate entity.
struct ClimateInfo : EntityInfo {
    std::vector<ClimateMode> supported_modes;
    float visual_min_temperature = 0.0F;
    float visual_max_temperature = 0.0F;
    float visual_target_temperature_step = 0.0F;
    std::vector<ClimateFanMode> supported_fan_modes;
    std::vector<ClimateSwingMode> supported_swing_modes;
    std::vector<std::string> supported_custom_fan_modes;
    std::vector<ClimatePreset> supported_presets;
    std::vector<std::string> supported_custom_presets;
    float visual_current_temperature_step = 0.0F;
    float visual_min_humidity = 0.0F;
    float visual_max_humidity = 0.0F;
    std::uint32_t feature_flags = 0;
    TemperatureUnit temperature_unit = TemperatureUnit::Celsius;
};

/// A climate entity's reported state.
struct ClimateState {
    std::uint32_t key = 0;
    ClimateMode mode = ClimateMode::Off;
    float current_temperature = 0.0F;
    float target_temperature = 0.0F;
    float target_temperature_low = 0.0F;
    float target_temperature_high = 0.0F;
    ClimateAction action = ClimateAction::Off;
    ClimateFanMode fan_mode = ClimateFanMode::On;
    ClimateSwingMode swing_mode = ClimateSwingMode::Off;
    std::string custom_fan_mode;
    ClimatePreset preset = ClimatePreset::None;
    std::string custom_preset;
    float current_humidity = 0.0F;
    float target_humidity = 0.0F;
};

/// A command to mutate a climate entity. Each optional field is sent only when engaged.
struct ClimateCommand {
    std::uint32_t key = 0;
    std::optional<ClimateMode> mode;
    std::optional<float> target_temperature;
    std::optional<float> target_temperature_low;
    std::optional<float> target_temperature_high;
    std::optional<ClimateFanMode> fan_mode;
    std::optional<ClimateSwingMode> swing_mode;
    std::optional<std::string> custom_fan_mode;
    std::optional<ClimatePreset> preset;
    std::optional<std::string> custom_preset;
    std::optional<float> target_humidity;
};

ClimateInfo parse_climate_info(const proto::ListEntitiesClimateResponse& msg);
ClimateState parse_climate_state(const proto::ClimateStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const ClimateCommand& cmd);

}  // namespace esphome::api
