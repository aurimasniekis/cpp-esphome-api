#include <esphome/api/model/entities/select.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

SelectInfo parse_select_info(const proto::ListEntitiesSelectResponse& msg) {
    SelectInfo info;
    fill_entity_info(info, msg);
    for (int i = 0; i < msg.options_size(); ++i) {
        info.options.push_back(msg.options(i));
    }
    return info;
}

SelectState parse_select_state(const proto::SelectStateResponse& msg) {
    SelectState state;
    state.key = msg.key();
    state.state = msg.state();
    state.missing_state = msg.missing_state();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const SelectCommand& cmd) {
    auto msg = std::make_unique<proto::SelectCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_state(cmd.state);
    return msg;
}

}  // namespace esphome::api
