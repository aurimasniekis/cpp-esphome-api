#pragma once

/// @file
/// @brief Typed AlarmControlPanel entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesAlarmControlPanelResponse;
class AlarmControlPanelStateResponse;
class AlarmControlPanelCommandRequest;
}  // namespace proto

/// Static description of an alarm control panel entity.
struct AlarmControlPanelInfo : EntityInfo {
    std::uint32_t supported_features = 0;
    bool requires_code = false;
    bool requires_code_to_arm = false;
};

/// An alarm control panel's reported state.
///
/// Named with the `Status` suffix because the canonical `AlarmControlPanelState`
/// name is already taken by the mirrored proto enum (see enums.hpp).
struct AlarmControlPanelStatus {
    std::uint32_t key = 0;
    AlarmControlPanelState state = AlarmControlPanelState::Disarmed;
};

/// A command to change an alarm control panel's state.
struct AlarmControlPanelCommand {
    std::uint32_t key = 0;
    AlarmControlPanelStateCommand command = AlarmControlPanelStateCommand::Disarm;
    std::string code;
};

AlarmControlPanelInfo
parse_alarm_control_panel_info(const proto::ListEntitiesAlarmControlPanelResponse& msg);
AlarmControlPanelStatus
parse_alarm_control_panel_state(const proto::AlarmControlPanelStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const AlarmControlPanelCommand& cmd);

}  // namespace esphome::api
