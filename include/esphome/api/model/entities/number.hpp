#pragma once

/// @file
/// @brief Typed Number entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesNumberResponse;
class NumberStateResponse;
class NumberCommandRequest;
}  // namespace proto

/// Static description of a number entity.
struct NumberInfo : EntityInfo {
    float min_value = 0.0F;
    float max_value = 0.0F;
    float step = 0.0F;
    std::string unit_of_measurement;
    NumberMode mode = NumberMode::Auto;
    std::string device_class;
};

/// A number's reported value.
struct NumberState {
    std::uint32_t key = 0;
    float state = 0.0F;
    /// True when the number currently has no valid value.
    bool missing_state = false;
};

/// A command to set a number's value.
struct NumberCommand {
    std::uint32_t key = 0;
    float state = 0.0F;
};

NumberInfo parse_number_info(const proto::ListEntitiesNumberResponse& msg);
NumberState parse_number_state(const proto::NumberStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const NumberCommand& cmd);

}  // namespace esphome::api
