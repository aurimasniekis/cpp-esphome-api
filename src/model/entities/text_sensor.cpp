#include <esphome/api/model/entities/text_sensor.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

TextSensorInfo parse_text_sensor_info(const proto::ListEntitiesTextSensorResponse& msg) {
    TextSensorInfo info;
    fill_entity_info(info, msg);
    info.device_class = msg.device_class();
    return info;
}

TextSensorState parse_text_sensor_state(const proto::TextSensorStateResponse& msg) {
    TextSensorState state;
    state.key = msg.key();
    state.state = msg.state();
    state.missing_state = msg.missing_state();
    return state;
}

}  // namespace esphome::api
