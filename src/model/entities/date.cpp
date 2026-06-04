#include <esphome/api/model/entities/date.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

DateInfo parse_date_info(const proto::ListEntitiesDateResponse& msg) {
    DateInfo info;
    fill_entity_info(info, msg);
    return info;
}

DateState parse_date_state(const proto::DateStateResponse& msg) {
    DateState state;
    state.key = msg.key();
    state.missing_state = msg.missing_state();
    state.year = msg.year();
    state.month = msg.month();
    state.day = msg.day();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const DateCommand& cmd) {
    auto msg = std::make_unique<proto::DateCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_year(cmd.year);
    msg->set_month(cmd.month);
    msg->set_day(cmd.day);
    return msg;
}

}  // namespace esphome::api
