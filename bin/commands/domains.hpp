#pragma once

/// @file
/// @brief Mapping between CLI domain command names and EntityType.

#include <esphome/api/model/entity_type.hpp>

#include <string>
#include <vector>

namespace cli {

struct DomainEntry {
    const char* command;  ///< snake_case CLI command, e.g. "binary_sensor".
    esphome::api::EntityType type;
};

/// All 26 entity domains, in a stable display order.
const std::vector<DomainEntry>& domains();

/// CLI command name -> EntityType (Unknown if not a domain command).
esphome::api::EntityType domain_to_type(const std::string& command);

/// EntityType -> CLI command name (empty for Unknown).
std::string type_to_domain(esphome::api::EntityType type);

}  // namespace cli
