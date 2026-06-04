#include <esphome/api/model/entities/update.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

UpdateInfo parse_update_info(const proto::ListEntitiesUpdateResponse& msg) {
    UpdateInfo info;
    fill_entity_info(info, msg);
    info.device_class = msg.device_class();
    return info;
}

UpdateState parse_update_state(const proto::UpdateStateResponse& msg) {
    UpdateState state;
    state.key = msg.key();
    state.missing_state = msg.missing_state();
    state.in_progress = msg.in_progress();
    state.has_progress = msg.has_progress();
    state.progress = msg.progress();
    state.current_version = msg.current_version();
    state.latest_version = msg.latest_version();
    state.title = msg.title();
    state.release_summary = msg.release_summary();
    state.release_url = msg.release_url();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const UpdateControl& cmd) {
    auto msg = std::make_unique<proto::UpdateCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_command(static_cast<proto::UpdateCommand>(cmd.command));
    return msg;
}

}  // namespace esphome::api
