#include <esphome/api/model/entities/infrared.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

InfraredInfo parse_infrared_info(const proto::ListEntitiesInfraredResponse& msg) {
    InfraredInfo info;
    fill_entity_info(info, msg);
    info.capabilities = msg.capabilities();
    info.receiver_frequency = msg.receiver_frequency();
    return info;
}

}  // namespace esphome::api
