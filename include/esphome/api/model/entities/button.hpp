#pragma once

/// @file
/// @brief Typed Button entity (info + command). A button has no state.

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesButtonResponse;
class ButtonCommandRequest;
}  // namespace proto

/// Static description of a button entity.
struct ButtonInfo : EntityInfo {
    std::string device_class;
};

/// A command to press a button.
struct ButtonCommand {
    std::uint32_t key = 0;
};

ButtonInfo parse_button_info(const proto::ListEntitiesButtonResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const ButtonCommand& cmd);

}  // namespace esphome::api
