#pragma once

/// @file
/// @brief Typed Date entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>

namespace esphome::api {

namespace proto {
class ListEntitiesDateResponse;
class DateStateResponse;
class DateCommandRequest;
}  // namespace proto

/// Static description of a date entity.
struct DateInfo : EntityInfo {};

/// A date's reported value.
struct DateState {
    std::uint32_t key = 0;
    /// True when the date currently has no valid value.
    bool missing_state = false;
    std::uint32_t year = 0;
    std::uint32_t month = 0;
    std::uint32_t day = 0;
};

/// A command to set a date's value.
struct DateCommand {
    std::uint32_t key = 0;
    std::uint32_t year = 0;
    std::uint32_t month = 0;
    std::uint32_t day = 0;
};

DateInfo parse_date_info(const proto::ListEntitiesDateResponse& msg);
DateState parse_date_state(const proto::DateStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const DateCommand& cmd);

}  // namespace esphome::api
