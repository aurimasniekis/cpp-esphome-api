#include <esphome/api/model/entities/siren.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

SirenInfo parse_siren_info(const proto::ListEntitiesSirenResponse& msg) {
    SirenInfo info;
    fill_entity_info(info, msg);
    for (int i = 0; i < msg.tones_size(); ++i) {
        info.tones.push_back(msg.tones(i));
    }
    info.supports_duration = msg.supports_duration();
    info.supports_volume = msg.supports_volume();
    return info;
}

SirenState parse_siren_state(const proto::SirenStateResponse& msg) {
    SirenState state;
    state.key = msg.key();
    state.state = msg.state();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const SirenCommand& cmd) {
    auto msg = std::make_unique<proto::SirenCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.state) {
        msg->set_has_state(true);
        msg->set_state(*cmd.state);
    }
    if (cmd.tone) {
        msg->set_has_tone(true);
        msg->set_tone(*cmd.tone);
    }
    if (cmd.duration) {
        msg->set_has_duration(true);
        msg->set_duration(*cmd.duration);
    }
    if (cmd.volume) {
        msg->set_has_volume(true);
        msg->set_volume(*cmd.volume);
    }
    return msg;
}

}  // namespace esphome::api
