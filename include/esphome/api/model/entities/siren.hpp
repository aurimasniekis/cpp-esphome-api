#pragma once

/// @file
/// @brief Typed Siren entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesSirenResponse;
class SirenStateResponse;
class SirenCommandRequest;
}  // namespace proto

/// Static description of a siren entity.
struct SirenInfo : EntityInfo {
    std::vector<std::string> tones;
    bool supports_duration = false;
    bool supports_volume = false;
};

/// A siren's reported state.
struct SirenState {
    std::uint32_t key = 0;
    bool state = false;
};

/// A command targeting a siren entity.
struct SirenCommand {
    std::uint32_t key = 0;
    std::optional<bool> state;
    std::optional<std::string> tone;
    std::optional<std::uint32_t> duration;
    std::optional<float> volume;
};

SirenInfo parse_siren_info(const proto::ListEntitiesSirenResponse& msg);
SirenState parse_siren_state(const proto::SirenStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const SirenCommand& cmd);

}  // namespace esphome::api
