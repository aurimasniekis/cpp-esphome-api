#include <esphome/api/model/entities/light.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

LightInfo parse_light_info(const proto::ListEntitiesLightResponse& msg) {
    LightInfo info;
    fill_entity_info(info, msg);
    for (int i = 0; i < msg.supported_color_modes_size(); ++i) {
        info.supported_color_modes.push_back(static_cast<ColorMode>(msg.supported_color_modes(i)));
    }
    info.min_mireds = msg.min_mireds();
    info.max_mireds = msg.max_mireds();
    for (int i = 0; i < msg.effects_size(); ++i) {
        info.effects.push_back(msg.effects(i));
    }
    return info;
}

LightState parse_light_state(const proto::LightStateResponse& msg) {
    LightState state;
    state.key = msg.key();
    state.state = msg.state();
    state.brightness = msg.brightness();
    state.color_mode = static_cast<ColorMode>(msg.color_mode());
    state.color_brightness = msg.color_brightness();
    state.red = msg.red();
    state.green = msg.green();
    state.blue = msg.blue();
    state.white = msg.white();
    state.color_temperature = msg.color_temperature();
    state.cold_white = msg.cold_white();
    state.warm_white = msg.warm_white();
    state.effect = msg.effect();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const LightCommand& cmd) {
    auto msg = std::make_unique<proto::LightCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.state) {
        msg->set_has_state(true);
        msg->set_state(*cmd.state);
    }
    if (cmd.brightness) {
        msg->set_has_brightness(true);
        msg->set_brightness(*cmd.brightness);
    }
    if (cmd.color_mode) {
        msg->set_has_color_mode(true);
        msg->set_color_mode(static_cast<proto::ColorMode>(*cmd.color_mode));
    }
    if (cmd.color_brightness) {
        msg->set_has_color_brightness(true);
        msg->set_color_brightness(*cmd.color_brightness);
    }
    if (cmd.rgb) {
        msg->set_has_rgb(true);
        msg->set_red(cmd.rgb->red);
        msg->set_green(cmd.rgb->green);
        msg->set_blue(cmd.rgb->blue);
    }
    if (cmd.white) {
        msg->set_has_white(true);
        msg->set_white(*cmd.white);
    }
    if (cmd.color_temperature) {
        msg->set_has_color_temperature(true);
        msg->set_color_temperature(*cmd.color_temperature);
    }
    if (cmd.cold_white) {
        msg->set_has_cold_white(true);
        msg->set_cold_white(*cmd.cold_white);
    }
    if (cmd.warm_white) {
        msg->set_has_warm_white(true);
        msg->set_warm_white(*cmd.warm_white);
    }
    if (cmd.transition_length) {
        msg->set_has_transition_length(true);
        msg->set_transition_length(*cmd.transition_length);
    }
    if (cmd.flash_length) {
        msg->set_has_flash_length(true);
        msg->set_flash_length(*cmd.flash_length);
    }
    if (cmd.effect) {
        msg->set_has_effect(true);
        msg->set_effect(*cmd.effect);
    }
    return msg;
}

}  // namespace esphome::api
