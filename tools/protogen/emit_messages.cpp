#include "emit.hpp"

#include <algorithm>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <unordered_set>

namespace protogen {

// ---------------------------------------------------------------------------
// Shared name / type helpers (also used by emit_enums / emit_registry).
// ---------------------------------------------------------------------------

std::string cpp_ident(const std::string& name) {
    static const std::unordered_set<std::string> k_keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
        "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
        "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
        "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
        "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
        "public", "register", "reinterpret_cast", "requires", "return", "short",
        "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
        "switch", "template", "this", "thread_local", "throw", "true", "try",
        "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
        "void", "volatile", "wchar_t", "while", "xor", "xor_eq"};
    if (k_keywords.count(name) != 0U)
        return name + "_";
    return name;
}

std::string cpp_element_type(const ProtoField& f) {
    switch (f.type) {
    case FieldType::Double: return "double";
    case FieldType::Float: return "float";
    case FieldType::Int32:
    case FieldType::Sfixed32:
    case FieldType::Sint32: return "std::int32_t";
    case FieldType::Int64:
    case FieldType::Sfixed64:
    case FieldType::Sint64: return "std::int64_t";
    case FieldType::Uint32:
    case FieldType::Fixed32: return "std::uint32_t";
    case FieldType::Uint64:
    case FieldType::Fixed64: return "std::uint64_t";
    case FieldType::Bool: return "bool";
    case FieldType::String:
    case FieldType::Bytes: return "std::string";
    case FieldType::Enum:
    case FieldType::Message: return cpp_ident(f.type_name);
    }
    return "std::int32_t";
}

namespace {

enum class Category { Varint, Fixed32, Fixed64, String, Bytes, Message };

Category category_of(const ProtoField& f) {
    switch (f.type) {
    case FieldType::Double:
    case FieldType::Fixed64:
    case FieldType::Sfixed64: return Category::Fixed64;
    case FieldType::Float:
    case FieldType::Fixed32:
    case FieldType::Sfixed32: return Category::Fixed32;
    case FieldType::String: return Category::String;
    case FieldType::Bytes: return Category::Bytes;
    case FieldType::Message: return Category::Message;
    default: return Category::Varint;  // int/uint/sint/bool/enum
    }
}

std::string member_name(const ProtoField& f) { return cpp_ident(f.name) + "_"; }
std::string presence_name(const ProtoField& f) { return "has_" + cpp_ident(f.name) + "_"; }

std::string wire_type_token(const Category c) {
    switch (c) {
    case Category::Varint: return "WireType::Varint";
    case Category::Fixed32: return "WireType::Fixed32";
    case Category::Fixed64: return "WireType::Fixed64";
    default: return "WireType::LengthDelimited";
    }
}

// Expression: encode a varint-family value `v` to its uint64 wire representation.
std::string varint_encode_expr(const ProtoField& f, const std::string& v) {
    switch (f.type) {
    case FieldType::Sint32: return "zigzag_encode32(" + v + ")";
    case FieldType::Sint64: return "zigzag_encode64(" + v + ")";
    case FieldType::Int32:
        return "static_cast<std::uint64_t>(static_cast<std::int64_t>(" + v + "))";
    case FieldType::Enum:
        return "static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<int>(" + v +
               ")))";
    case FieldType::Int64: return "static_cast<std::uint64_t>(" + v + ")";
    case FieldType::Bool: return "(" + v + " ? 1u : 0u)";
    default: return "static_cast<std::uint64_t>(" + v + ")";  // uint32/uint64
    }
}

// Statement: write value `v` (no tag) for a non-message field.
std::string write_value_stmt(const ProtoField& f, const std::string& v) {
    switch (category_of(f)) {
    case Category::Varint:
        return "w.varint(" + varint_encode_expr(f, v) + ");";
    case Category::Fixed32:
        if (f.type == FieldType::Float)
            return "w.fixed32(float_to_bits(" + v + "));";
        if (f.type == FieldType::Sfixed32)
            return "w.fixed32(static_cast<std::uint32_t>(" + v + "));";
        return "w.fixed32(" + v + ");";
    case Category::Fixed64:
        if (f.type == FieldType::Double)
            return "w.fixed64(double_to_bits(" + v + "));";
        if (f.type == FieldType::Sfixed64)
            return "w.fixed64(static_cast<std::uint64_t>(" + v + "));";
        return "w.fixed64(" + v + ");";
    default:  // String / Bytes
        return "w.length_delimited(" + v + ");";
    }
}

// Expression: byte length of value `v` for a non-message, non-string field.
std::string value_size_expr(const ProtoField& f, const std::string& v) {
    switch (category_of(f)) {
    case Category::Varint: return "varint_size(" + varint_encode_expr(f, v) + ")";
    case Category::Fixed32: return "4";
    case Category::Fixed64: return "8";
    default: return "(varint_size(" + v + ".size()) + " + v + ".size())";  // string/bytes
    }
}

// Non-default condition for a singular field's member.
std::string nondefault_cond(const ProtoField& f, const std::string& m) {
    switch (f.type) {
    case FieldType::Bool: return m;
    case FieldType::Enum: return "static_cast<int>(" + m + ") != 0";
    case FieldType::String:
    case FieldType::Bytes: return "!" + m + ".empty()";
    default: return m + " != 0";
    }
}

// Reader call + raw type for a varint/fixed element.
struct ReadInfo {
    std::string raw_type;
    std::string read_call;  // e.g. "varint", "fixed32", "fixed64"
};
ReadInfo read_info(const Category c) {
    switch (c) {
    case Category::Varint: return {"std::uint64_t", "varint"};
    case Category::Fixed32: return {"std::uint32_t", "fixed32"};
    default: return {"std::uint64_t", "fixed64"};  // Fixed64
    }
}

// Expression converting a raw decoded value `v` into the stored element type.
std::string decode_convert_expr(const ProtoField& f, const std::string& v) {
    switch (f.type) {
    case FieldType::Uint32:
    case FieldType::Fixed32: return "static_cast<std::uint32_t>(" + v + ")";
    case FieldType::Uint64:
    case FieldType::Fixed64: return v;
    case FieldType::Int32:
    case FieldType::Sfixed32: return "static_cast<std::int32_t>(" + v + ")";
    case FieldType::Int64:
    case FieldType::Sfixed64: return "static_cast<std::int64_t>(" + v + ")";
    case FieldType::Bool: return "(" + v + " != 0)";
    case FieldType::Enum:
        return "static_cast<" + cpp_ident(f.type_name) + ">(static_cast<std::int32_t>(" + v + "))";
    case FieldType::Sint32: return "zigzag_decode32(static_cast<std::uint32_t>(" + v + "))";
    case FieldType::Sint64: return "zigzag_decode64(" + v + ")";
    case FieldType::Float: return "bits_to_float(" + v + ")";
    case FieldType::Double: return "bits_to_double(" + v + ")";
    default: return v;
    }
}

bool getter_overrides(const ProtoField& f, const std::string& base) {
    if (base == "InfoResponseProtoMessage")
        return f.name == "key" || f.name == "name" || f.name == "object_id";
    if (base == "StateResponseProtoMessage")
        return f.name == "key";
    return false;
}

std::string base_specifier(const std::string& base) {
    if (base == "InfoResponseProtoMessage")
        return "InfoResponseBase";
    if (base == "StateResponseProtoMessage")
        return "StateResponseBase";
    return "ProtoMessage";
}

std::vector<const ProtoField*> sorted_fields(const ProtoMessageDef& m) {
    std::vector<const ProtoField*> fs;
    fs.reserve(m.fields.size());
    for (const auto& f : m.fields)
        fs.push_back(&f);
    std::sort(fs.begin(), fs.end(),
              [](const ProtoField* a, const ProtoField* b) { return a->number < b->number; });
    return fs;
}

// ---- header: accessors ----------------------------------------------------

void emit_accessors(std::ostream& os, const ProtoField& f, const std::string& base) {
    const std::string g = cpp_ident(f.name);
    const std::string m = member_name(f);
    const std::string elem = cpp_element_type(f);
    const std::string ov = getter_overrides(f, base) ? " override" : "";

    if (!f.repeated) {
        switch (category_of(f)) {
        case Category::String:
        case Category::Bytes:
            os << "    const std::string& " << g << "() const" << ov << " { return " << m
               << "; }\n";
            os << "    void set_" << g << "(const std::string& value) { " << m << " = value; }\n";
            os << "    void set_" << g << "(std::string&& value) { " << m
               << " = std::move(value); }\n";
            os << "    void set_" << g << "(const char* value) { " << m << " = value; }\n";
            break;
        case Category::Message:
            os << "    const " << elem << "& " << g << "() const { return " << m << "; }\n";
            os << "    " << elem << "* mutable_" << g << "() { " << presence_name(f)
               << " = true; return &" << m << "; }\n";
            os << "    bool has_" << g << "() const { return " << presence_name(f) << "; }\n";
            break;
        default:  // scalar / enum
            os << "    " << elem << " " << g << "() const" << ov << " { return " << m << "; }\n";
            os << "    void set_" << g << "(" << elem << " value) { " << m << " = value; }\n";
            break;
        }
        return;
    }

    // Repeated.
    os << "    int " << g << "_size() const { return static_cast<int>(" << m << ".size()); }\n";
    switch (category_of(f)) {
    case Category::String:
    case Category::Bytes:
        os << "    const std::string& " << g << "(int index) const { return " << m
           << "[static_cast<std::size_t>(index)]; }\n";
        os << "    void add_" << g << "(const std::string& value) { " << m
           << ".push_back(value); }\n";
        os << "    void add_" << g << "(const char* value) { " << m << ".emplace_back(value); }\n";
        break;
    case Category::Message:
        os << "    const " << elem << "& " << g << "(int index) const { return " << m
           << "[static_cast<std::size_t>(index)]; }\n";
        os << "    " << elem << "* add_" << g << "() { " << m << ".emplace_back(); return &" << m
           << ".back(); }\n";
        break;
    default:  // scalar / enum
        os << "    " << elem << " " << g << "(int index) const { return " << m
           << "[static_cast<std::size_t>(index)]; }\n";
        os << "    void add_" << g << "(" << elem << " value) { " << m << ".push_back(value); }\n";
        break;
    }
}

void emit_member(std::ostream& os, const ProtoField& f) {
    const std::string m = member_name(f);
    const std::string elem = cpp_element_type(f);
    if (f.repeated) {
        os << "    std::vector<" << elem << "> " << m << ";\n";
        return;
    }
    switch (category_of(f)) {
    case Category::String:
    case Category::Bytes:
        os << "    std::string " << m << ";\n";
        break;
    case Category::Message:
        os << "    " << elem << " " << m << "{};\n";
        os << "    bool " << presence_name(f) << "{false};\n";
        break;
    default:
        os << "    " << elem << " " << m << "{};\n";
        break;
    }
}

// ---- cpp: encode ----------------------------------------------------------

void emit_encode_field(std::ostream& os, const ProtoField& f) {
    const std::string m = member_name(f);
    const int n = f.number;
    const Category c = category_of(f);

    if (!f.repeated) {
        if (c == Category::Message) {
            os << "    if (" << presence_name(f) << ") {\n";
            os << "        w.tag(" << n << ", WireType::LengthDelimited);\n";
            os << "        w.varint(" << m << ".calculate_size());\n";
            os << "        " << m << ".encode(w);\n";
            os << "    }\n";
            return;
        }
        os << "    if (" << nondefault_cond(f, m) << ") {\n";
        os << "        w.tag(" << n << ", " << wire_type_token(c) << ");\n";
        os << "        " << write_value_stmt(f, m) << "\n";
        os << "    }\n";
        return;
    }

    // Repeated.
    if (c == Category::Message) {
        os << "    for (const auto& v : " << m << ") {\n";
        os << "        w.tag(" << n << ", WireType::LengthDelimited);\n";
        os << "        w.varint(v.calculate_size());\n";
        os << "        v.encode(w);\n";
        os << "    }\n";
        return;
    }
    if (c == Category::String || c == Category::Bytes) {
        os << "    for (const auto& v : " << m << ") {\n";
        os << "        w.tag(" << n << ", WireType::LengthDelimited);\n";
        os << "        w.length_delimited(v);\n";
        os << "    }\n";
        return;
    }
    // Repeated numeric: packed or unpacked.
    if (is_packed(f)) {
        os << "    if (!" << m << ".empty()) {\n";
        if (c == Category::Fixed32) {
            os << "        const std::size_t packed_size = " << m << ".size() * 4;\n";
        } else if (c == Category::Fixed64) {
            os << "        const std::size_t packed_size = " << m << ".size() * 8;\n";
        } else {
            os << "        std::size_t packed_size = 0;\n";
            os << "        for (const auto& v : " << m << ") packed_size += "
               << value_size_expr(f, "v") << ";\n";
        }
        os << "        w.tag(" << n << ", WireType::LengthDelimited);\n";
        os << "        w.varint(packed_size);\n";
        os << "        for (const auto& v : " << m << ") " << write_value_stmt(f, "v") << "\n";
        os << "    }\n";
        return;
    }
    // Unpacked repeated numeric.
    os << "    for (const auto& v : " << m << ") {\n";
    os << "        w.tag(" << n << ", " << wire_type_token(c) << ");\n";
    os << "        " << write_value_stmt(f, "v") << "\n";
    os << "    }\n";
}

// ---- cpp: calculate_size --------------------------------------------------

void emit_size_field(std::ostream& os, const ProtoField& f) {
    const std::string m = member_name(f);
    const int n = f.number;
    const Category c = category_of(f);

    if (!f.repeated) {
        if (c == Category::Message) {
            os << "    if (" << presence_name(f) << ") {\n";
            os << "        const std::size_t s = " << m << ".calculate_size();\n";
            os << "        total += tag_size(" << n << ") + varint_size(s) + s;\n";
            os << "    }\n";
            return;
        }
        os << "    if (" << nondefault_cond(f, m) << ")\n";
        os << "        total += tag_size(" << n << ") + " << value_size_expr(f, m) << ";\n";
        return;
    }

    if (c == Category::Message) {
        os << "    for (const auto& v : " << m << ") {\n";
        os << "        const std::size_t s = v.calculate_size();\n";
        os << "        total += tag_size(" << n << ") + varint_size(s) + s;\n";
        os << "    }\n";
        return;
    }
    if (c == Category::String || c == Category::Bytes) {
        os << "    for (const auto& v : " << m << ")\n";
        os << "        total += tag_size(" << n << ") + varint_size(v.size()) + v.size();\n";
        return;
    }
    if (is_packed(f)) {
        os << "    if (!" << m << ".empty()) {\n";
        if (c == Category::Fixed32) {
            os << "        const std::size_t packed_size = " << m << ".size() * 4;\n";
        } else if (c == Category::Fixed64) {
            os << "        const std::size_t packed_size = " << m << ".size() * 8;\n";
        } else {
            os << "        std::size_t packed_size = 0;\n";
            os << "        for (const auto& v : " << m << ") packed_size += "
               << value_size_expr(f, "v") << ";\n";
        }
        os << "        total += tag_size(" << n << ") + varint_size(packed_size) + packed_size;\n";
        os << "    }\n";
        return;
    }
    os << "    for (const auto& v : " << m << ")\n";
    os << "        total += tag_size(" << n << ") + " << value_size_expr(f, "v") << ";\n";
}

// ---- cpp: decode ----------------------------------------------------------

void emit_decode_field(std::ostream& os, const ProtoField& f) {
    const std::string m = member_name(f);
    const int n = f.number;
    const Category c = category_of(f);

    os << "        case " << n << ": {\n";

    if (c == Category::Message) {
        os << "            ProtoReader sub;\n";
        os << "            if (!r.sub_reader(sub)) return false;\n";
        if (f.repeated) {
            os << "            " << m << ".emplace_back();\n";
            os << "            if (!" << m << ".back().decode(sub)) return false;\n";
        } else {
            os << "            if (!" << m << ".decode(sub)) return false;\n";
            os << "            " << presence_name(f) << " = true;\n";
        }
        os << "            break;\n        }\n";
        return;
    }

    if (c == Category::String || c == Category::Bytes) {
        if (f.repeated) {
            os << "            std::string v;\n";
            os << "            if (!r.length_delimited(v)) return false;\n";
            os << "            " << m << ".push_back(std::move(v));\n";
        } else {
            os << "            if (!r.length_delimited(" << m << ")) return false;\n";
        }
        os << "            break;\n        }\n";
        return;
    }

    const auto [raw_type, read_call] = read_info(c);
    if (f.repeated) {
        os << "            if (wt == WireType::LengthDelimited) {\n";
        os << "                ProtoReader sub;\n";
        os << "                if (!r.sub_reader(sub)) return false;\n";
        os << "                while (!sub.eof()) {\n";
        os << "                    " << raw_type << " v;\n";
        os << "                    if (!sub." << read_call << "(v)) return false;\n";
        os << "                    " << m << ".push_back(" << decode_convert_expr(f, "v") << ");\n";
        os << "                }\n";
        os << "                if (!sub.ok()) return false;\n";
        os << "            } else {\n";
        os << "                " << raw_type << " v;\n";
        os << "                if (!r." << read_call << "(v)) return false;\n";
        os << "                " << m << ".push_back(" << decode_convert_expr(f, "v") << ");\n";
        os << "            }\n";
    } else {
        os << "            " << raw_type << " v;\n";
        os << "            if (!r." << read_call << "(v)) return false;\n";
        os << "            " << m << " = " << decode_convert_expr(f, "v") << ";\n";
    }
    os << "            break;\n        }\n";
}

// ---- whole-message emission -----------------------------------------------

void emit_class_decl(std::ostream& os, const ProtoMessageDef& m) {
    const std::string cls = cpp_ident(m.name);
    os << "class " << cls << " : public " << base_specifier(m.base_class) << " {\n";
    os << "public:\n";
    for (const auto& f : m.fields)
        emit_accessors(os, f, m.base_class);
    os << "\n";
    os << "    void encode(ProtoWriter& w) const override;\n";
    os << "    bool decode(ProtoReader& r) override;\n";
    os << "    std::size_t calculate_size() const override;\n";
    os << "    std::uint32_t message_id() const override { return " << m.id << "; }\n";
    os << "    const char* message_name() const override { return \"" << m.name << "\"; }\n";
    os << "    std::unique_ptr<ProtoMessage> clone() const override {\n";
    os << "        return std::make_unique<" << cls << ">(*this);\n";
    os << "    }\n";
    if (!m.fields.empty()) {
        os << "\nprivate:\n";
        for (const auto& f : m.fields)
            emit_member(os, f);
    }
    os << "};\n\n";
}

void emit_class_defs(std::ostream& os, const ProtoMessageDef& m) {
    const std::string cls = cpp_ident(m.name);
    const auto fields = sorted_fields(m);

    os << "void " << cls << "::encode(ProtoWriter& w) const {\n";
    if (m.fields.empty())
        os << "    (void)w;\n";
    for (const auto* f : fields)
        emit_encode_field(os, *f);
    os << "}\n\n";

    os << "std::size_t " << cls << "::calculate_size() const {\n";
    os << "    std::size_t total = 0;\n";
    for (const auto* f : fields)
        emit_size_field(os, *f);
    os << "    return total;\n";
    os << "}\n\n";

    os << "bool " << cls << "::decode(ProtoReader& r) {\n";
    if (m.fields.empty()) {
        os << "    std::uint32_t field_number = 0;\n";
        os << "    WireType wt{};\n";
        os << "    while (r.read_tag(field_number, wt)) {\n";
        os << "        if (!r.skip_field(wt)) return false;\n";
        os << "    }\n";
        os << "    return r.ok();\n";
        os << "}\n\n";
        return;
    }
    os << "    std::uint32_t field_number = 0;\n";
    os << "    WireType wt{};\n";
    os << "    while (r.read_tag(field_number, wt)) {\n";
    os << "        switch (field_number) {\n";
    for (const auto* f : fields)
        emit_decode_field(os, *f);
    os << "        default:\n";
    os << "            if (!r.skip_field(wt)) return false;\n";
    os << "            break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "    return r.ok();\n";
    os << "}\n\n";
}

}  // namespace

void emit_messages(const ProtoFile& file, const std::string& include_dir,
                   const std::string& src_dir) {
    // Header.
    {
        const std::string path = include_dir + "/api_messages.hpp";
        std::ofstream os(path, std::ios::binary | std::ios::trunc);
        if (!os)
            throw std::runtime_error("cannot open " + path);
        os << "// Generated by tools/protogen — do not edit.\n";
        os << "#pragma once\n\n";
        os << "#include <esphome/api/proto/api_enums.hpp>\n";
        os << "#include <esphome/api/proto/proto_message.hpp>\n\n";
        os << "#include <cstddef>\n#include <cstdint>\n#include <memory>\n";
        os << "#include <string>\n#include <vector>\n\n";
        os << "namespace esphome::api::proto {\n\n";
        for (const auto& m : file.messages)
            emit_class_decl(os, m);

        // Registry entry points (defined in api_registry.cpp).
        os << "// Registry helpers (id <-> message), defined in api_registry.cpp.\n";
        os << "std::unique_ptr<ProtoMessage> create_message(std::uint32_t id);\n";
        os << "bool registry_contains(std::uint32_t id);\n";
        os << "std::size_t registry_size();\n";
        os << "const char* registry_name(std::uint32_t id);\n\n";
        os << "}  // namespace esphome::api::proto\n";
    }

    // Source.
    {
        const std::string path = src_dir + "/api_messages.cpp";
        std::ofstream os(path, std::ios::binary | std::ios::trunc);
        if (!os)
            throw std::runtime_error("cannot open " + path);
        os << "// Generated by tools/protogen — do not edit.\n";
        os << "#include <esphome/api/proto/api_messages.hpp>\n\n";
        os << "#include <utility>\n\n";
        os << "namespace esphome::api::proto {\n\n";
        for (const auto& m : file.messages)
            emit_class_defs(os, m);
        os << "}  // namespace esphome::api::proto\n";
    }
}

}  // namespace protogen
