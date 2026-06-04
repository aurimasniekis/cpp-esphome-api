#include <esphome/api/model/entities/valve.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

ValveInfo parse_valve_info(const proto::ListEntitiesValveResponse& msg) {
    ValveInfo info;
    fill_entity_info(info, msg);
    info.device_class = msg.device_class();
    info.assumed_state = msg.assumed_state();
    info.supports_position = msg.supports_position();
    info.supports_stop = msg.supports_stop();
    return info;
}

ValveState parse_valve_state(const proto::ValveStateResponse& msg) {
    ValveState state;
    state.key = msg.key();
    state.position = msg.position();
    state.current_operation = static_cast<ValveOperation>(msg.current_operation());
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const ValveCommand& cmd) {
    auto msg = std::make_unique<proto::ValveCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.position) {
        msg->set_has_position(true);
        msg->set_position(*cmd.position);
    }
    msg->set_stop(cmd.stop);
    return msg;
}

}  // namespace esphome::api
