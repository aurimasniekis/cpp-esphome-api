#include <esphome/api/model/entities/text.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

TextInfo parse_text_info(const proto::ListEntitiesTextResponse& msg) {
    TextInfo info;
    fill_entity_info(info, msg);
    info.min_length = msg.min_length();
    info.max_length = msg.max_length();
    info.pattern = msg.pattern();
    info.mode = static_cast<TextMode>(msg.mode());
    return info;
}

TextState parse_text_state(const proto::TextStateResponse& msg) {
    TextState state;
    state.key = msg.key();
    state.state = msg.state();
    state.missing_state = msg.missing_state();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const TextCommand& cmd) {
    auto msg = std::make_unique<proto::TextCommandRequest>();
    msg->set_key(cmd.key);
    msg->set_state(cmd.state);
    return msg;
}

}  // namespace esphome::api
