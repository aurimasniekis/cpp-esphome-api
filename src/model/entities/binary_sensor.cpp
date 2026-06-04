#include <esphome/api/model/entities/binary_sensor.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

BinarySensorInfo parse_binary_sensor_info(const proto::ListEntitiesBinarySensorResponse& msg) {
    BinarySensorInfo info;
    fill_entity_info(info, msg);
    info.device_class = msg.device_class();
    info.is_status_binary_sensor = msg.is_status_binary_sensor();
    return info;
}

BinarySensorState parse_binary_sensor_state(const proto::BinarySensorStateResponse& msg) {
    BinarySensorState state;
    state.key = msg.key();
    state.state = msg.state();
    state.missing_state = msg.missing_state();
    return state;
}

}  // namespace esphome::api
