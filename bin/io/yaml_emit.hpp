#pragma once

/// @file
/// @brief Minimal block-style YAML emitter built on nlohmann::json. No yaml-cpp.

#include <nlohmann/json.hpp>

#include <string>

namespace cli {

/// Render a JSON document as block-style YAML (no trailing newline).
std::string to_yaml(const nlohmann::json& doc);

/// Render a single JSON value as one YAML sequence item ("- ..."), used for
/// incremental streaming output (one record per line group).
std::string to_yaml_list_item(const nlohmann::json& value);

}  // namespace cli
