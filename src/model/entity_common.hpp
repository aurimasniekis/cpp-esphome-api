#pragma once

/// @file
/// @brief Internal helper to copy the shared EntityInfo fields from any
///        ListEntities*Response into an EntityInfo. Private to the library
///        (includes api.pb.h).

#include <esphome/api/model/entity.hpp>

#include "api.pb.h"

#include <cctype>
#include <string>

namespace esphome::api {

/// Derive an entity's object_id from its name, matching ESPHome's
/// `str_sanitize(str_snake_case(name))`: lowercase, spaces become underscores,
/// then anything outside [a-z0-9_-] is dropped. Used when the device does not
/// send `object_id` (removed from the API in ESPHome PR #12698 / API ≥ 1.14).
inline std::string object_id_from_name(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (const char ch : name) {
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (c == ' ')
            c = '_';
        if (c == '-' || c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))
            out.push_back(c);
    }
    return out;
}

/// Populate the common EntityInfo fields from a ListEntities*Response. The
/// concrete message type is templated since every such message shares these
/// accessors (object_id/key/name/icon/disabled_by_default/entity_category/
/// device_id). Falls back to deriving object_id from name when the device does
/// not send one (API ≥ 1.14).
template <class ListMsg>
void fill_entity_info(EntityInfo& info, const ListMsg& msg) {
    info.key = msg.key();
    info.name = msg.name();
    info.object_id = msg.object_id();
    if (info.object_id.empty())
        info.object_id = object_id_from_name(info.name);
    info.icon = msg.icon();
    info.disabled_by_default = msg.disabled_by_default();
    info.entity_category = static_cast<EntityCategory>(msg.entity_category());
    info.device_id = msg.device_id();
}

}  // namespace esphome::api
