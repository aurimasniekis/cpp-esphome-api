#pragma once

/// @file
/// @brief Typed Cover entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesCoverResponse;
class CoverStateResponse;
class CoverCommandRequest;
}  // namespace proto

/// Static description of a cover entity.
struct CoverInfo : EntityInfo {
    bool assumed_state = false;
    bool supports_position = false;
    bool supports_tilt = false;
    std::string device_class;
    bool supports_stop = false;
};

/// A cover's reported state.
struct CoverState {
    std::uint32_t key = 0;
    float position = 0.0F;
    float tilt = 0.0F;
    CoverOperation current_operation = CoverOperation::Idle;
};

/// A command targeting a cover entity.
struct CoverCommand {
    std::uint32_t key = 0;
    std::optional<float> position;
    std::optional<float> tilt;
    bool stop = false;
};

CoverInfo parse_cover_info(const proto::ListEntitiesCoverResponse& msg);
CoverState parse_cover_state(const proto::CoverStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const CoverCommand& cmd);

}  // namespace esphome::api
