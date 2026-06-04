#pragma once

/// @file
/// @brief Table of (domain, action) -> command verb, plus value parsers.

#include <esphome/api/model/entities/light.hpp>
#include <esphome/api/model/entity_type.hpp>
#include <esphome/api/sync_client.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace cli {

/// Outcome of an entity action.
struct ActionResult {
    bool ok = false;
    std::string message;
};

/// Resolve a typed handle from `client` for `object_id` and invoke a verb.
/// May throw std::exception on bad arguments (the caller reports it).
using ActionFn = std::function<ActionResult(esphome::api::SyncClient& client,
                                            const std::string& object_id,
                                            const std::vector<std::string>& values)>;

/// Look up the action for a domain; returns nullptr if the action is unknown.
const ActionFn* find_action(esphome::api::EntityType type, const std::string& action);

/// The valid action names for a domain (sorted), for help / error messages.
std::vector<std::string> actions_for(esphome::api::EntityType type);

/// actions_for() plus the universal read-only verbs "info" and "state" that
/// every entity supports, sorted.
std::vector<std::string> available_actions(esphome::api::EntityType type);

// --- Value parsers (throw std::invalid_argument on malformed input) ---------

/// "50%" -> 0.5, "0.5" -> 0.5. Clamped to [0, 1].
float parse_percent_or_unit(const std::string& text);
/// true/on/yes/1 vs false/off/no/0 (case-insensitive).
bool parse_bool(const std::string& text);
float parse_float(const std::string& text);
std::int32_t parse_int(const std::string& text);
std::uint32_t parse_uint(const std::string& text);
/// "r,g,b" (one token) or three tokens; components 0-255 or 0-1, mapped to 0-1.
esphome::api::LightRgb parse_rgb(const std::vector<std::string>& values);
/// "AA:BB:CC:DD:EE:FF", "0x...", or decimal -> 48-bit BLE address.
std::uint64_t parse_ble_addr(const std::string& text);

}  // namespace cli
