#pragma once

/// @file
/// @brief Common entity metadata shared by every entity domain.

#include <esphome/api/model/entity_type.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>
#include <string>

namespace esphome::api {

/// Fields present on every ListEntities*Response. Each domain's `<X>Info`
/// struct derives from this and adds its own metadata.
struct EntityInfo {
    /// Stable per-entity identifier used to correlate state and command messages.
    std::uint32_t key = 0;
    std::string object_id;
    std::string name;
    std::string icon;
    bool disabled_by_default = false;
    EntityCategory entity_category = EntityCategory::None;
    /// Sub-device id (0 = the root device) when the device exposes sub-devices.
    std::uint32_t device_id = 0;
};

}  // namespace esphome::api
