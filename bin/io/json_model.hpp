#pragma once

/// @file
/// @brief Assemble per-entity JSON documents (domain + info + state) using the
///        library's public to_json serializers, plus compact text helpers.

#include <esphome/api/model/entity_store.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace cli {

/// Build a JSON document for one stored entity: {domain, object_id, name, key,
/// info, state?}. When `include_state` is false the "state" field is omitted.
nlohmann::json entity_to_json(const esphome::api::EntityStore& store,
                              const esphome::api::StoredEntity& entity,
                              bool include_state);

/// Compact one-line summary of an entity's "state" object (as produced by
/// entity_to_json). Returns "-" when no state is present.
std::string state_summary(const nlohmann::json& entity_doc);

/// A describe-style body for one entity: {info, state, actions} with the
/// internal numeric "key" stripped (state is null when the entity has none).
nlohmann::json entity_detail(const esphome::api::EntityStore& store,
                             const esphome::api::StoredEntity& entity);

}  // namespace cli
