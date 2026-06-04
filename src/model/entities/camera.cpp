#include <esphome/api/model/entities/camera.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

CameraInfo parse_camera_info(const proto::ListEntitiesCameraResponse& msg) {
    CameraInfo info;
    fill_entity_info(info, msg);
    return info;
}

}  // namespace esphome::api
