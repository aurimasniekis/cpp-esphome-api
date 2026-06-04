#include <esphome/api/model/entities/button.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

ButtonInfo parse_button_info(const proto::ListEntitiesButtonResponse& msg) {
    ButtonInfo info;
    fill_entity_info(info, msg);
    info.device_class = msg.device_class();
    return info;
}

std::unique_ptr<ProtoMessage> to_message(const ButtonCommand& cmd) {
    auto msg = std::make_unique<proto::ButtonCommandRequest>();
    msg->set_key(cmd.key);
    return msg;
}

}  // namespace esphome::api
