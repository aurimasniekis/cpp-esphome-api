#include <esphome/api/model/entities/event.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

EventInfo parse_event_info(const proto::ListEntitiesEventResponse& msg) {
    EventInfo info;
    fill_entity_info(info, msg);
    info.device_class = msg.device_class();
    for (int i = 0; i < msg.event_types_size(); ++i) {
        info.event_types.push_back(msg.event_types(i));
    }
    return info;
}

EventState parse_event_state(const proto::EventResponse& msg) {
    EventState state;
    state.key = msg.key();
    state.event_type = msg.event_type();
    return state;
}

}  // namespace esphome::api
