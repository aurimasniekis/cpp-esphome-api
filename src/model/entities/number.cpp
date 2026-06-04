#include <esphome/api/model/entities/number.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

NumberInfo parse_number_info(const proto::ListEntitiesNumberResponse& msg) {
    NumberInfo info;
    fill_entity_info(info, msg);
    info.min_value = msg.min_value();
    info.max_value = msg.max_value();
    info.step = msg.step();
    info.unit_of_measurement = msg.unit_of_measurement();
    info.mode = static_cast<NumberMode>(msg.mode());
    info.device_class = msg.device_class();
    return info;
}

NumberState parse_number_state(const proto::NumberStateResponse& msg) {
    NumberState state;
    state.key = msg.key();
    state.state = msg.state();
    state.missing_state = msg.missing_state();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const NumberCommand& cmd) {
    auto msg = std::make_unique<proto::NumberCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_state(cmd.state);
    return msg;
}

}  // namespace esphome::api
