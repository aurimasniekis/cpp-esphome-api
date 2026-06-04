#include <esphome/api/model/entities/sensor.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

SensorInfo parse_sensor_info(const proto::ListEntitiesSensorResponse& msg) {
    SensorInfo info;
    fill_entity_info(info, msg);
    info.unit_of_measurement = msg.unit_of_measurement();
    info.accuracy_decimals = msg.accuracy_decimals();
    info.force_update = msg.force_update();
    info.device_class = msg.device_class();
    info.state_class = static_cast<SensorStateClass>(msg.state_class());
    return info;
}

SensorState parse_sensor_state(const proto::SensorStateResponse& msg) {
    SensorState state;
    state.key = msg.key();
    state.state = msg.state();
    state.missing_state = msg.missing_state();
    return state;
}

}  // namespace esphome::api
