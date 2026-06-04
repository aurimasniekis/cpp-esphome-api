#include <esphome/api/model/entities/switch.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

SwitchInfo parse_switch_info(const proto::ListEntitiesSwitchResponse& msg) {
    SwitchInfo info;
    fill_entity_info(info, msg);
    info.assumed_state = msg.assumed_state();
    info.device_class = msg.device_class();
    return info;
}

SwitchState parse_switch_state(const proto::SwitchStateResponse& msg) {
    SwitchState state;
    state.key = msg.key();
    state.state = msg.state();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const SwitchCommand& cmd) {
    auto msg = std::make_unique<proto::SwitchCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_state(cmd.state);
    return msg;
}

}  // namespace esphome::api
