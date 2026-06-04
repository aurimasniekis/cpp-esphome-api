#pragma once

/// @file
/// @brief protogen recursive-descent parser for the proto3 subset. Hard-errors
///        on any construct outside the subset (oneof / map / group / nested
///        types / unknown keywords), so a future re-vendor fails loudly instead
///        of miscompiling.

#include "lexer.hpp"
#include "model.hpp"

#include <string>
#include <vector>

namespace protogen {

/// Parse one proto file's tokens, appending its enums/messages to `file`.
/// Throws std::runtime_error on any unsupported construct.
void parse_into(const std::vector<Token>& tokens, const std::string& filename, ProtoFile& file);

/// Resolve every field's type_name to FieldType::Enum / FieldType::Message
/// (or a scalar). Throws if a type name is unknown.
void resolve_types(ProtoFile& file);

}  // namespace protogen
