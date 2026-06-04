#include <esphome/api/model/entities/lock.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

LockInfo parse_lock_info(const proto::ListEntitiesLockResponse& msg) {
    LockInfo info;
    fill_entity_info(info, msg);
    info.assumed_state = msg.assumed_state();
    info.supports_open = msg.supports_open();
    info.requires_code = msg.requires_code();
    info.code_format = msg.code_format();
    return info;
}

LockEntityState parse_lock_state(const proto::LockStateResponse& msg) {
    LockEntityState state;
    state.key = msg.key();
    state.state = static_cast<LockState>(msg.state());
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const LockCommandData& cmd) {
    auto msg = std::make_unique<proto::LockCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_command(static_cast<proto::LockCommand>(cmd.command));
    if (cmd.code) {
        msg->set_has_code(true);
        msg->set_code(*cmd.code);
    }
    return msg;
}

}  // namespace esphome::api
