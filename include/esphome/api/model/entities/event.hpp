#pragma once

/// @file
/// @brief Typed Event entity (info + state).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesEventResponse;
class EventResponse;
}  // namespace proto

/// Static description of an event entity.
struct EventInfo : EntityInfo {
    std::string device_class;
    std::vector<std::string> event_types;
};

/// An event's reported value.
struct EventState {
    std::uint32_t key = 0;
    std::string event_type;
};

EventInfo parse_event_info(const proto::ListEntitiesEventResponse& msg);
EventState parse_event_state(const proto::EventResponse& msg);

}  // namespace esphome::api
