#pragma once

/// @file
/// @brief Lightweight forward declarations for the protobuf layer, so public
///        headers can refer to messages without pulling in the large
///        generated `api.pb.h`.

namespace google::protobuf {
class Message;
class Descriptor;
}  // namespace google::protobuf

namespace esphome::api {

/// The protobuf base class every generated ESPHome message derives from. The
/// concrete types live in namespace `esphome::api::proto` (see `api.pb.h`),
/// reachable through the raw escape hatch on the client.
using ProtoMessage = google::protobuf::Message;

}  // namespace esphome::api
