#pragma once

/// @file
/// @brief Typed DateTime entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>

namespace esphome::api {

namespace proto {
class ListEntitiesDateTimeResponse;
class DateTimeStateResponse;
class DateTimeCommandRequest;
}  // namespace proto

/// Static description of a datetime entity.
struct DateTimeInfo : EntityInfo {};

/// A datetime's reported value.
struct DateTimeState {
    std::uint32_t key = 0;
    /// True when the datetime currently has no valid value.
    bool missing_state = false;
    std::uint32_t epoch_seconds = 0;
};

/// A command to set a datetime's value.
struct DateTimeCommand {
    std::uint32_t key = 0;
    std::uint32_t epoch_seconds = 0;
};

DateTimeInfo parse_datetime_info(const proto::ListEntitiesDateTimeResponse& msg);
DateTimeState parse_datetime_state(const proto::DateTimeStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const DateTimeCommand& cmd);

}  // namespace esphome::api
