#pragma once

/// @file
/// @brief Typed Valve entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesValveResponse;
class ValveStateResponse;
class ValveCommandRequest;
}  // namespace proto

/// Static description of a valve entity.
struct ValveInfo : EntityInfo {
    std::string device_class;
    bool assumed_state = false;
    bool supports_position = false;
    bool supports_stop = false;
};

/// A valve's reported state.
struct ValveState {
    std::uint32_t key = 0;
    float position = 0.0F;
    ValveOperation current_operation = ValveOperation::Idle;
};

/// A command targeting a valve entity.
struct ValveCommand {
    std::uint32_t key = 0;
    std::optional<float> position;
    bool stop = false;
};

ValveInfo parse_valve_info(const proto::ListEntitiesValveResponse& msg);
ValveState parse_valve_state(const proto::ValveStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const ValveCommand& cmd);

}  // namespace esphome::api
