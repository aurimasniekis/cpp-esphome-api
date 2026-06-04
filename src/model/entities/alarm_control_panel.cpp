#include <esphome/api/model/entities/alarm_control_panel.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

AlarmControlPanelInfo
parse_alarm_control_panel_info(const proto::ListEntitiesAlarmControlPanelResponse& msg) {
    AlarmControlPanelInfo info;
    fill_entity_info(info, msg);
    info.supported_features = msg.supported_features();
    info.requires_code = msg.requires_code();
    info.requires_code_to_arm = msg.requires_code_to_arm();
    return info;
}

AlarmControlPanelStatus
parse_alarm_control_panel_state(const proto::AlarmControlPanelStateResponse& msg) {
    AlarmControlPanelStatus state;
    state.key = msg.key();
    state.state = static_cast<AlarmControlPanelState>(msg.state());
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const AlarmControlPanelCommand& cmd) {
    auto msg = std::make_unique<proto::AlarmControlPanelCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_command(static_cast<proto::AlarmControlPanelStateCommand>(cmd.command));
    msg->set_code(cmd.code);
    return msg;
}

}  // namespace esphome::api
