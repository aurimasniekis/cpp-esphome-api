#pragma once

/// @file
/// @brief Typed Text entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesTextResponse;
class TextStateResponse;
class TextCommandRequest;
}  // namespace proto

/// Static description of a text entity.
struct TextInfo : EntityInfo {
    std::uint32_t min_length = 0;
    std::uint32_t max_length = 0;
    std::string pattern;
    TextMode mode = TextMode::Text;
};

/// A text entity's reported value.
struct TextState {
    std::uint32_t key = 0;
    std::string state;
    /// True when the text entity currently has no valid value.
    bool missing_state = false;
};

/// A command to set a text entity's value.
struct TextCommand {
    std::uint32_t key = 0;
    std::string state;
};

TextInfo parse_text_info(const proto::ListEntitiesTextResponse& msg);
TextState parse_text_state(const proto::TextStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const TextCommand& cmd);

}  // namespace esphome::api
