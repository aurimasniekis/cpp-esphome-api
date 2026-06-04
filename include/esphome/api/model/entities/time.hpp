#pragma once

/// @file
/// @brief Typed Time entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>

namespace esphome::api {

namespace proto {
class ListEntitiesTimeResponse;
class TimeStateResponse;
class TimeCommandRequest;
}  // namespace proto

/// Static description of a time entity.
struct TimeInfo : EntityInfo {};

/// A time's reported value.
struct TimeState {
    std::uint32_t key = 0;
    /// True when the time currently has no valid value.
    bool missing_state = false;
    std::uint32_t hour = 0;
    std::uint32_t minute = 0;
    std::uint32_t second = 0;
};

/// A command to set a time's value.
struct TimeCommand {
    std::uint32_t key = 0;
    std::uint32_t hour = 0;
    std::uint32_t minute = 0;
    std::uint32_t second = 0;
};

TimeInfo parse_time_info(const proto::ListEntitiesTimeResponse& msg);
TimeState parse_time_state(const proto::TimeStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const TimeCommand& cmd);

}  // namespace esphome::api
