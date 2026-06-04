#include <esphome/api/model/entities/water_heater.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

WaterHeaterInfo parse_water_heater_info(const proto::ListEntitiesWaterHeaterResponse& msg) {
    WaterHeaterInfo info;
    fill_entity_info(info, msg);
    info.min_temperature = msg.min_temperature();
    info.max_temperature = msg.max_temperature();
    info.target_temperature_step = msg.target_temperature_step();
    for (int i = 0; i < msg.supported_modes_size(); ++i) {
        info.supported_modes.push_back(static_cast<WaterHeaterMode>(msg.supported_modes(i)));
    }
    info.supported_features = msg.supported_features();
    info.temperature_unit = static_cast<TemperatureUnit>(msg.temperature_unit());
    return info;
}

WaterHeaterState parse_water_heater_state(const proto::WaterHeaterStateResponse& msg) {
    WaterHeaterState state;
    state.key = msg.key();
    state.current_temperature = msg.current_temperature();
    state.target_temperature = msg.target_temperature();
    state.mode = static_cast<WaterHeaterMode>(msg.mode());
    state.state = msg.state();
    state.target_temperature_low = msg.target_temperature_low();
    state.target_temperature_high = msg.target_temperature_high();
    return state;
}

std::unique_ptr<ProtoMessage> to_message(const WaterHeaterCommand& cmd) {
    auto msg = std::make_unique<proto::WaterHeaterCommandRequest>();
    msg->set_key(cmd.key);

    std::uint32_t has_fields = 0;
    std::uint32_t state = 0;

    if (cmd.mode) {
        has_fields |= static_cast<std::uint32_t>(proto::WATER_HEATER_COMMAND_HAS_MODE);
        msg->set_mode(static_cast<proto::WaterHeaterMode>(*cmd.mode));
    }
    if (cmd.target_temperature) {
        has_fields |=
            static_cast<std::uint32_t>(proto::WATER_HEATER_COMMAND_HAS_TARGET_TEMPERATURE);
        msg->set_target_temperature(*cmd.target_temperature);
    }
    if (cmd.target_temperature_low) {
        has_fields |=
            static_cast<std::uint32_t>(proto::WATER_HEATER_COMMAND_HAS_TARGET_TEMPERATURE_LOW);
        msg->set_target_temperature_low(*cmd.target_temperature_low);
    }
    if (cmd.target_temperature_high) {
        has_fields |=
            static_cast<std::uint32_t>(proto::WATER_HEATER_COMMAND_HAS_TARGET_TEMPERATURE_HIGH);
        msg->set_target_temperature_high(*cmd.target_temperature_high);
    }
    if (cmd.on_state) {
        has_fields |= static_cast<std::uint32_t>(proto::WATER_HEATER_COMMAND_HAS_ON_STATE);
        if (*cmd.on_state) {
            state |= 0x2U;  // bit 1 = on
        }
    }
    if (cmd.away_state) {
        has_fields |= static_cast<std::uint32_t>(proto::WATER_HEATER_COMMAND_HAS_AWAY_STATE);
        if (*cmd.away_state) {
            state |= 0x1U;  // bit 0 = away
        }
    }

    msg->set_has_fields(has_fields);
    msg->set_state(state);
    return msg;
}

}  // namespace esphome::api
