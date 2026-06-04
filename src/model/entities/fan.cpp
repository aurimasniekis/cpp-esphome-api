#include <esphome/api/model/entities/fan.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

FanInfo parse_fan_info(const proto::ListEntitiesFanResponse& msg) {
    FanInfo info;
    fill_entity_info(info, msg);
    info.supports_oscillation = msg.supports_oscillation();
    info.supports_speed = msg.supports_speed();
    info.supports_direction = msg.supports_direction();
    info.supported_speed_count = msg.supported_speed_count();
    for (int i = 0; i < msg.supported_preset_modes_size(); ++i) {
        info.supported_preset_modes.push_back(msg.supported_preset_modes(i));
    }
    return info;
}

FanState parse_fan_state(const proto::FanStateResponse& msg) {
    FanState state;
    state.key = msg.key();
    state.state = msg.state();
    state.oscillating = msg.oscillating();
    state.direction = static_cast<FanDirection>(msg.direction());
    state.speed_level = msg.speed_level();
    state.preset_mode = msg.preset_mode();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const FanCommand& cmd) {
    auto msg = std::make_unique<proto::FanCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.state) {
        msg->set_has_state(true);
        msg->set_state(*cmd.state);
    }
    if (cmd.oscillating) {
        msg->set_has_oscillating(true);
        msg->set_oscillating(*cmd.oscillating);
    }
    if (cmd.direction) {
        msg->set_has_direction(true);
        msg->set_direction(static_cast<proto::FanDirection>(*cmd.direction));
    }
    if (cmd.speed_level) {
        msg->set_has_speed_level(true);
        msg->set_speed_level(*cmd.speed_level);
    }
    if (cmd.preset_mode) {
        msg->set_has_preset_mode(true);
        msg->set_preset_mode(*cmd.preset_mode);
    }
    return msg;
}

}  // namespace esphome::api
