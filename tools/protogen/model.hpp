#pragma once

/// @file
/// @brief protogen AST for the proto3 subset the ESPHome native API uses —
///        populated by the parser, consumed by the emitters.

#include <cstdint>
#include <string>
#include <vector>

namespace protogen {

enum class FieldType {
    Double,
    Float,
    Int32,
    Int64,
    Uint32,
    Uint64,
    Sint32,
    Sint64,
    Fixed32,
    Fixed64,
    Sfixed32,
    Sfixed64,
    Bool,
    String,
    Bytes,
    Enum,     // type_name names a ProtoEnum
    Message,  // type_name names a ProtoMessageDef
};

struct ProtoField {
    std::string name;       // field name as written (already C++-safe in the proto)
    std::string type_name;  // raw type token ("uint32", "ColorMode", "DeviceInfo", ...)
    FieldType type = FieldType::Int32;
    int number = 0;
    bool repeated = false;

    // Packing for repeated numeric fields. `packed_explicit` records an explicit
    // [packed=...]; otherwise proto3's default (packed for scalar numerics).
    bool packed_explicit = false;
    bool packed_value = false;
};

struct ProtoEnumValue {
    std::string name;
    std::int64_t number = 0;
};

struct ProtoEnum {
    std::string name;
    std::vector<ProtoEnumValue> values;
};

struct ProtoMessageDef {
    std::string name;
    std::vector<ProtoField> fields;
    std::uint32_t id = 0;    // (id) option, 0 if absent
    std::string base_class;  // (base_class) option, empty if absent
};

struct ProtoFile {
    std::string package;
    std::vector<ProtoEnum> enums;
    std::vector<ProtoMessageDef> messages;
};

// --- type helpers ----------------------------------------------------------

/// True for the 15 proto3 scalar keywords.
bool is_scalar_type(const std::string& t);

/// Wire type number (0/1/2/5) for a resolved field.
int wire_type_of(const ProtoField& f);

/// True if a repeated field of this type is packed (honoring explicit flag).
bool is_packed(const ProtoField& f);

}  // namespace protogen
