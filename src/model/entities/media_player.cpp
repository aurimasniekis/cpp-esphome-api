#include <esphome/api/model/entities/media_player.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

MediaPlayerInfo parse_media_player_info(const proto::ListEntitiesMediaPlayerResponse& msg) {
    MediaPlayerInfo info;
    fill_entity_info(info, msg);
    info.supports_pause = msg.supports_pause();
    for (int i = 0; i < msg.supported_formats_size(); ++i) {
        const auto& fmt = msg.supported_formats(i);
        MediaPlayerSupportedFormat out;
        out.format = fmt.format();
        out.sample_rate = fmt.sample_rate();
        out.num_channels = fmt.num_channels();
        out.purpose = static_cast<MediaPlayerFormatPurpose>(fmt.purpose());
        out.sample_bytes = fmt.sample_bytes();
        info.supported_formats.push_back(std::move(out));
    }
    return info;
}

MediaPlayerStatus parse_media_player_state(const proto::MediaPlayerStateResponse& msg) {
    MediaPlayerStatus state;
    state.key = msg.key();
    state.state = static_cast<MediaPlayerState>(msg.state());
    state.volume = msg.volume();
    state.muted = msg.muted();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const MediaPlayerControl& cmd) {
    auto msg = std::make_unique<proto::MediaPlayerCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.command) {
        msg->set_has_command(true);
        msg->set_command(static_cast<proto::MediaPlayerCommand>(*cmd.command));
    }
    if (cmd.volume) {
        msg->set_has_volume(true);
        msg->set_volume(*cmd.volume);
    }
    if (cmd.media_url) {
        msg->set_has_media_url(true);
        msg->set_media_url(*cmd.media_url);
    }
    if (cmd.announcement) {
        msg->set_has_announcement(true);
        msg->set_announcement(*cmd.announcement);
    }
    return msg;
}

}  // namespace esphome::api
