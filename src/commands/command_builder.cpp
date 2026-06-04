#include <esphome/api/commands/command_builder.hpp>

#include "api.pb.h"

#include <memory>

#include <google/protobuf/message.h>

namespace esphome::api {

void request_camera_image(Client& client, const bool single, const bool stream) {
    proto::CameraImageRequest req;
    req.set_single(single);
    req.set_stream(stream);
    client.send(req);
}

void send_command(Client& client, const SwitchCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const ButtonCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const NumberCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const SelectCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const TextCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const LightCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const FanCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const CoverCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const ValveCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const LockCommandData& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const SirenCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const ClimateCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const WaterHeaterCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const MediaPlayerControl& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const AlarmControlPanelCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const DateCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const TimeCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const DateTimeCommand& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

void send_command(Client& client, const UpdateControl& cmd) {
    const auto msg = to_message(cmd);
    client.send(*msg);
}

}  // namespace esphome::api
