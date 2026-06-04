#pragma once

/// @file
/// @brief Typed Select entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesSelectResponse;
class SelectStateResponse;
class SelectCommandRequest;
}  // namespace proto

/// Static description of a select entity.
struct SelectInfo : EntityInfo {
    std::vector<std::string> options;
};

/// A select's reported value.
struct SelectState {
    std::uint32_t key = 0;
    std::string state;
    /// True when the select currently has no valid value.
    bool missing_state = false;
};

/// A command to set a select's value.
struct SelectCommand {
    std::uint32_t key = 0;
    std::string state;
};

SelectInfo parse_select_info(const proto::ListEntitiesSelectResponse& msg);
SelectState parse_select_state(const proto::SelectStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const SelectCommand& cmd);

}  // namespace esphome::api
