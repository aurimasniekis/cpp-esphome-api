#include <esphome/api/model/entities/climate.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

ClimateInfo parse_climate_info(const proto::ListEntitiesClimateResponse& msg) {
    ClimateInfo info;
    fill_entity_info(info, msg);
    for (int i = 0; i < msg.supported_modes_size(); ++i) {
        info.supported_modes.push_back(static_cast<ClimateMode>(msg.supported_modes(i)));
    }
    info.visual_min_temperature = msg.visual_min_temperature();
    info.visual_max_temperature = msg.visual_max_temperature();
    info.visual_target_temperature_step = msg.visual_target_temperature_step();
    for (int i = 0; i < msg.supported_fan_modes_size(); ++i) {
        info.supported_fan_modes.push_back(static_cast<ClimateFanMode>(msg.supported_fan_modes(i)));
    }
    for (int i = 0; i < msg.supported_swing_modes_size(); ++i) {
        info.supported_swing_modes.push_back(
            static_cast<ClimateSwingMode>(msg.supported_swing_modes(i)));
    }
    for (int i = 0; i < msg.supported_custom_fan_modes_size(); ++i) {
        info.supported_custom_fan_modes.push_back(msg.supported_custom_fan_modes(i));
    }
    for (int i = 0; i < msg.supported_presets_size(); ++i) {
        info.supported_presets.push_back(static_cast<ClimatePreset>(msg.supported_presets(i)));
    }
    for (int i = 0; i < msg.supported_custom_presets_size(); ++i) {
        info.supported_custom_presets.push_back(msg.supported_custom_presets(i));
    }
    info.visual_current_temperature_step = msg.visual_current_temperature_step();
    info.visual_min_humidity = msg.visual_min_humidity();
    info.visual_max_humidity = msg.visual_max_humidity();
    info.feature_flags = msg.feature_flags();
    info.temperature_unit = static_cast<TemperatureUnit>(msg.temperature_unit());
    return info;
}

ClimateState parse_climate_state(const proto::ClimateStateResponse& msg) {
    ClimateState state;
    state.key = msg.key();
    state.mode = static_cast<ClimateMode>(msg.mode());
    state.current_temperature = msg.current_temperature();
    state.target_temperature = msg.target_temperature();
    state.target_temperature_low = msg.target_temperature_low();
    state.target_temperature_high = msg.target_temperature_high();
    state.action = static_cast<ClimateAction>(msg.action());
    state.fan_mode = static_cast<ClimateFanMode>(msg.fan_mode());
    state.swing_mode = static_cast<ClimateSwingMode>(msg.swing_mode());
    state.custom_fan_mode = msg.custom_fan_mode();
    state.preset = static_cast<ClimatePreset>(msg.preset());
    state.custom_preset = msg.custom_preset();
    state.current_humidity = msg.current_humidity();
    state.target_humidity = msg.target_humidity();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const ClimateCommand& cmd) {
    auto msg = std::make_unique<proto::ClimateCommandRequest>();
    msg->set_key(cmd.key);
    if (cmd.mode) {
        msg->set_has_mode(true);
        msg->set_mode(static_cast<proto::ClimateMode>(*cmd.mode));
    }
    if (cmd.target_temperature) {
        msg->set_has_target_temperature(true);
        msg->set_target_temperature(*cmd.target_temperature);
    }
    if (cmd.target_temperature_low) {
        msg->set_has_target_temperature_low(true);
        msg->set_target_temperature_low(*cmd.target_temperature_low);
    }
    if (cmd.target_temperature_high) {
        msg->set_has_target_temperature_high(true);
        msg->set_target_temperature_high(*cmd.target_temperature_high);
    }
    if (cmd.fan_mode) {
        msg->set_has_fan_mode(true);
        msg->set_fan_mode(static_cast<proto::ClimateFanMode>(*cmd.fan_mode));
    }
    if (cmd.swing_mode) {
        msg->set_has_swing_mode(true);
        msg->set_swing_mode(static_cast<proto::ClimateSwingMode>(*cmd.swing_mode));
    }
    if (cmd.custom_fan_mode) {
        msg->set_has_custom_fan_mode(true);
        msg->set_custom_fan_mode(*cmd.custom_fan_mode);
    }
    if (cmd.preset) {
        msg->set_has_preset(true);
        msg->set_preset(static_cast<proto::ClimatePreset>(*cmd.preset));
    }
    if (cmd.custom_preset) {
        msg->set_has_custom_preset(true);
        msg->set_custom_preset(*cmd.custom_preset);
    }
    if (cmd.target_humidity) {
        msg->set_has_target_humidity(true);
        msg->set_target_humidity(*cmd.target_humidity);
    }
    return msg;
}

}  // namespace esphome::api
