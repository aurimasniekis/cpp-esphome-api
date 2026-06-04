#pragma once

/// @file
/// @brief Typed Camera entity (info only; images are streamed separately).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

namespace esphome::api {

namespace proto {
class ListEntitiesCameraResponse;
}  // namespace proto

/// Static description of a camera entity.
struct CameraInfo : EntityInfo {};

CameraInfo parse_camera_info(const proto::ListEntitiesCameraResponse& msg);

}  // namespace esphome::api
