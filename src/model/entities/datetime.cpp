#include <esphome/api/model/entities/datetime.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

DateTimeInfo parse_datetime_info(const proto::ListEntitiesDateTimeResponse& msg) {
    DateTimeInfo info;
    fill_entity_info(info, msg);
    return info;
}

DateTimeState parse_datetime_state(const proto::DateTimeStateResponse& msg) {
    DateTimeState state;
    state.key = msg.key();
    state.missing_state = msg.missing_state();
    state.epoch_seconds = msg.epoch_seconds();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const DateTimeCommand& cmd) {
    auto msg = std::make_unique<proto::DateTimeCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_epoch_seconds(cmd.epoch_seconds);
    return msg;
}

}  // namespace esphome::api
