#pragma once

/// @file
/// @brief Typed Fan entity (info + state + command).

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
class ListEntitiesFanResponse;
class FanStateResponse;
class FanCommandRequest;
}  // namespace proto

/// Static description of a fan entity.
struct FanInfo : EntityInfo {
    bool supports_oscillation = false;
    bool supports_speed = false;
    bool supports_direction = false;
    std::int32_t supported_speed_count = 0;
    std::vector<std::string> supported_preset_modes;
};

/// A fan's reported state.
struct FanState {
    std::uint32_t key = 0;
    bool state = false;
    bool oscillating = false;
    FanDirection direction = FanDirection::Forward;
    std::int32_t speed_level = 0;
    std::string preset_mode;
};

/// A command to mutate a fan. Each optional field is sent only when engaged.
struct FanCommand {
    std::uint32_t key = 0;
    std::optional<bool> state;
    std::optional<bool> oscillating;
    std::optional<FanDirection> direction;
    std::optional<std::int32_t> speed_level;
    std::optional<std::string> preset_mode;
};

FanInfo parse_fan_info(const proto::ListEntitiesFanResponse& msg);
FanState parse_fan_state(const proto::FanStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const FanCommand& cmd);

}  // namespace esphome::api
