#include <esphome/api/model/entities/time.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

TimeInfo parse_time_info(const proto::ListEntitiesTimeResponse& msg) {
    TimeInfo info;
    fill_entity_info(info, msg);
    return info;
}

TimeState parse_time_state(const proto::TimeStateResponse& msg) {
    TimeState state;
    state.key = msg.key();
    state.missing_state = msg.missing_state();
    state.hour = msg.hour();
    state.minute = msg.minute();
    state.second = msg.second();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const TimeCommand& cmd) {
    auto msg = std::make_unique<proto::TimeCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_hour(cmd.hour);
    msg->set_minute(cmd.minute);
    msg->set_second(cmd.second);
    return msg;
}

}  // namespace esphome::api
