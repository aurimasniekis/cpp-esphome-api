#pragma once

/// @file
/// @brief Typed MediaPlayer entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class ListEntitiesMediaPlayerResponse;
class MediaPlayerStateResponse;
class MediaPlayerCommandRequest;
}  // namespace proto

/// A single audio format advertised as supported by a media player.
struct MediaPlayerSupportedFormat {
    std::string format;
    std::uint32_t sample_rate = 0;
    std::uint32_t num_channels = 0;
    MediaPlayerFormatPurpose purpose = MediaPlayerFormatPurpose::Default;
    std::uint32_t sample_bytes = 0;
};

/// Static description of a media player entity.
struct MediaPlayerInfo : EntityInfo {
    bool supports_pause = false;
    std::vector<MediaPlayerSupportedFormat> supported_formats;
};

/// A media player's reported state.
///
/// Named with the `Status` suffix because the canonical `MediaPlayerState`
/// name is already taken by the mirrored proto enum (see enums.hpp).
struct MediaPlayerStatus {
    std::uint32_t key = 0;
    MediaPlayerState state = MediaPlayerState::None;
    float volume = 0.0F;
    bool muted = false;
};

/// A command to mutate a media player. Each optional field is sent only when engaged.
///
/// Named with the `Control` suffix because the canonical `MediaPlayerCommand`
/// name is already taken by the mirrored proto enum (see enums.hpp).
struct MediaPlayerControl {
    std::uint32_t key = 0;
    std::optional<MediaPlayerCommand> command;
    std::optional<float> volume;
    std::optional<std::string> media_url;
    std::optional<bool> announcement;
};

MediaPlayerInfo parse_media_player_info(const proto::ListEntitiesMediaPlayerResponse& msg);
MediaPlayerStatus parse_media_player_state(const proto::MediaPlayerStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const MediaPlayerControl& cmd);

}  // namespace esphome::api
