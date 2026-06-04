#pragma once

/// @file
/// @brief Brings the lightweight ProtoMessage base into scope for public
///        headers, without pulling in the large generated `api.pb.h`.
///
/// Historically this declared `esphome::api::ProtoMessage` as an alias for
/// `google::protobuf::Message`. After the protoc -> protogen migration the base
/// is a self-contained class defined in proto_message.hpp; this header is kept
/// so the many includes of it stay valid.

#include <esphome/api/proto/proto_message.hpp>

namespace esphome::api {

// `ProtoMessage` is defined in proto_message.hpp (included above). The concrete
// message types live in namespace `esphome::api::proto` (see `api.pb.h`).

}  // namespace esphome::api
