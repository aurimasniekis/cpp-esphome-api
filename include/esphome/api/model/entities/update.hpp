#pragma once

/// @file
/// @brief Typed Update entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesUpdateResponse;
class UpdateStateResponse;
class UpdateCommandRequest;
}  // namespace proto

/// Static description of an update entity.
struct UpdateInfo : EntityInfo {
    std::string device_class;
};

/// An update's reported value.
struct UpdateState {
    std::uint32_t key = 0;
    /// True when the update currently has no valid value.
    bool missing_state = false;
    bool in_progress = false;
    bool has_progress = false;
    float progress = 0.0F;
    std::string current_version;
    std::string latest_version;
    std::string title;
    std::string release_summary;
    std::string release_url;
};

/// A command to control an update entity. (Named `UpdateControl` to avoid
/// clashing with the mirrored `UpdateCommand` enum.)
struct UpdateControl {
    std::uint32_t key = 0;
    UpdateCommand command = UpdateCommand::None;
};

UpdateInfo parse_update_info(const proto::ListEntitiesUpdateResponse& msg);
UpdateState parse_update_state(const proto::UpdateStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const UpdateControl& cmd);

}  // namespace esphome::api
