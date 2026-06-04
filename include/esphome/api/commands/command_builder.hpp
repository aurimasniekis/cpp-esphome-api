#pragma once

/// @file
/// @brief Convenience senders that translate a typed command struct into its
///        protobuf CommandRequest and dispatch it over a Client.

#include <esphome/api/client.hpp>
#include <esphome/api/model/entities/alarm_control_panel.hpp>
#include <esphome/api/model/entities/button.hpp>
#include <esphome/api/model/entities/climate.hpp>
#include <esphome/api/model/entities/cover.hpp>
#include <esphome/api/model/entities/date.hpp>
#include <esphome/api/model/entities/datetime.hpp>
#include <esphome/api/model/entities/fan.hpp>
#include <esphome/api/model/entities/light.hpp>
#include <esphome/api/model/entities/lock.hpp>
#include <esphome/api/model/entities/media_player.hpp>
#include <esphome/api/model/entities/number.hpp>
#include <esphome/api/model/entities/select.hpp>
#include <esphome/api/model/entities/siren.hpp>
#include <esphome/api/model/entities/switch.hpp>
#include <esphome/api/model/entities/text.hpp>
#include <esphome/api/model/entities/time.hpp>
#include <esphome/api/model/entities/update.hpp>
#include <esphome/api/model/entities/valve.hpp>
#include <esphome/api/model/entities/water_heater.hpp>

namespace esphome::api {

/// Request a camera still image (single) or start/stop a stream. Cameras are
/// addressed globally by the protocol, so no key is needed.
void request_camera_image(const Client& client, bool single, bool stream);

/// Send a SwitchCommand to the device.
void send_command(const Client& client, const SwitchCommand& cmd);
/// Send a ButtonCommand to the device.
void send_command(const Client& client, const ButtonCommand& cmd);
/// Send a NumberCommand to the device.
void send_command(const Client& client, const NumberCommand& cmd);
/// Send a SelectCommand to the device.
void send_command(const Client& client, const SelectCommand& cmd);
/// Send a TextCommand to the device.
void send_command(const Client& client, const TextCommand& cmd);
/// Send a LightCommand to the device.
void send_command(const Client& client, const LightCommand& cmd);
/// Send a FanCommand to the device.
void send_command(const Client& client, const FanCommand& cmd);
/// Send a CoverCommand to the device.
void send_command(const Client& client, const CoverCommand& cmd);
/// Send a ValveCommand to the device.
void send_command(const Client& client, const ValveCommand& cmd);
/// Send a LockCommandData to the device.
void send_command(const Client& client, const LockCommandData& cmd);
/// Send a SirenCommand to the device.
void send_command(const Client& client, const SirenCommand& cmd);
/// Send a ClimateCommand to the device.
void send_command(const Client& client, const ClimateCommand& cmd);
/// Send a WaterHeaterCommand to the device.
void send_command(const Client& client, const WaterHeaterCommand& cmd);
/// Send a MediaPlayerControl to the device.
void send_command(const Client& client, const MediaPlayerControl& cmd);
/// Send a AlarmControlPanelCommand to the device.
void send_command(const Client& client, const AlarmControlPanelCommand& cmd);
/// Send a DateCommand to the device.
void send_command(const Client& client, const DateCommand& cmd);
/// Send a TimeCommand to the device.
void send_command(const Client& client, const TimeCommand& cmd);
/// Send a DateTimeCommand to the device.
void send_command(const Client& client, const DateTimeCommand& cmd);
/// Send a UpdateControl to the device.
void send_command(const Client& client, const UpdateControl& cmd);

}  // namespace esphome::api
