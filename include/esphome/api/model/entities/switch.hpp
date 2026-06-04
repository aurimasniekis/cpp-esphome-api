#pragma once

/// @file
/// @brief Typed Switch entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesSwitchResponse;
class SwitchStateResponse;
class SwitchCommandRequest;
}  // namespace proto

/// Static description of a switch entity.
struct SwitchInfo : EntityInfo {
    bool assumed_state = false;
    std::string device_class;
};

/// A switch's reported state.
struct SwitchState {
    std::uint32_t key = 0;
    bool state = false;
};

/// A command to change a switch's state.
struct SwitchCommand {
    std::uint32_t key = 0;
    bool state = false;
};

SwitchInfo parse_switch_info(const proto::ListEntitiesSwitchResponse& msg);
SwitchState parse_switch_state(const proto::SwitchStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const SwitchCommand& cmd);

}  // namespace esphome::api
