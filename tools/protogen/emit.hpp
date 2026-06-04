#pragma once

/// @file
/// @brief protogen code emitters. Each writes one or more generated files
///        (enums, message classes, the id<->type registry, message_id.hpp)
///        from the parsed model.

#include "model.hpp"

#include <string>

namespace protogen {

// Append "_" to C++ keywords (e.g. proto message "void" -> "void_"), following
// the standard proto -> C++ name mangling. Other names are already C++-safe in
// the vendored proto.
std::string cpp_ident(const std::string& name);

// C++ element type for a field (the scalar / enum / message type, ignoring
// `repeated`). Strings and bytes map to std::string.
std::string cpp_element_type(const ProtoField& f);

// generated/include/esphome/api/proto/api_enums.hpp
void emit_enums(const ProtoFile& file, const std::string& out_dir);

// generated/include/esphome/api/proto/api_messages.hpp  (+ src/api_messages.cpp)
void emit_messages(const ProtoFile& file, const std::string& include_dir,
                   const std::string& src_dir);

// generated/src/api_registry.cpp + generated/include/esphome/api/proto/message_id.hpp
void emit_registry(const ProtoFile& file, const std::string& include_dir,
                   const std::string& src_dir);

}  // namespace protogen
