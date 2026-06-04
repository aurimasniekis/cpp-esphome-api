#pragma once

/// @file
/// @brief Typed Light entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesLightResponse;
class LightStateResponse;
class LightCommandRequest;
}  // namespace proto

/// Static description of a light entity.
struct LightInfo : EntityInfo {
    std::vector<ColorMode> supported_color_modes;
    float min_mireds = 0.0F;
    float max_mireds = 0.0F;
    std::vector<std::string> effects;
};

/// A light's reported state.
struct LightState {
    std::uint32_t key = 0;
    bool state = false;
    float brightness = 0.0F;
    ColorMode color_mode = ColorMode::Unknown;
    float color_brightness = 0.0F;
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
    float white = 0.0F;
    float color_temperature = 0.0F;
    float cold_white = 0.0F;
    float warm_white = 0.0F;
    std::string effect;
};

/// RGB triple, set together via the single `has_rgb` command flag.
struct LightRgb {
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
};

/// A command to mutate a light. Each optional field is sent only when engaged.
struct LightCommand {
    std::uint32_t key = 0;
    std::optional<bool> state;
    std::optional<float> brightness;
    std::optional<ColorMode> color_mode;
    std::optional<float> color_brightness;
    std::optional<LightRgb> rgb;
    std::optional<float> white;
    std::optional<float> color_temperature;
    std::optional<float> cold_white;
    std::optional<float> warm_white;
    std::optional<std::uint32_t> transition_length;
    std::optional<std::uint32_t> flash_length;
    std::optional<std::string> effect;
};

LightInfo parse_light_info(const proto::ListEntitiesLightResponse& msg);
LightState parse_light_state(const proto::LightStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const LightCommand& cmd);

}  // namespace esphome::api
