#include <esphome/api/model/entities/cover.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

CoverInfo parse_cover_info(const proto::ListEntitiesCoverResponse& msg) {
    CoverInfo info;
    fill_entity_info(info, msg);
    info.assumed_state = msg.assumed_state();
    info.supports_position = msg.supports_position();
    info.supports_tilt = msg.supports_tilt();
    info.device_class = msg.device_class();
    info.supports_stop = msg.supports_stop();
    return info;
}

CoverState parse_cover_state(const proto::CoverStateResponse& msg) {
    CoverState state;
    state.key = msg.key();
    state.position = msg.position();
    state.tilt = msg.tilt();
    state.current_operation = static_cast<CoverOperation>(msg.current_operation());
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const CoverCommand& cmd) {
    auto msg = std::make_unique<proto::CoverCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.position) {
        msg->set_has_position(true);
        msg->set_position(*cmd.position);
    }
    if (cmd.tilt) {
        msg->set_has_tilt(true);
        msg->set_tilt(*cmd.tilt);
    }
    msg->set_stop(cmd.stop);
    return msg;
}

}  // namespace esphome::api
