#include "parser.hpp"

#include <array>
#include <stdexcept>
#include <unordered_set>

namespace protogen {

namespace {

const std::array<const char*, 15> kScalarTypes = {
    "double", "float",  "int32",    "int64",    "uint32",
    "uint64", "sint32", "sint64",   "fixed32",  "fixed64",
    "sfixed32", "sfixed64", "bool", "string",   "bytes"};

FieldType scalar_field_type(const std::string& t) {
    if (t == "double") return FieldType::Double;
    if (t == "float") return FieldType::Float;
    if (t == "int32") return FieldType::Int32;
    if (t == "int64") return FieldType::Int64;
    if (t == "uint32") return FieldType::Uint32;
    if (t == "uint64") return FieldType::Uint64;
    if (t == "sint32") return FieldType::Sint32;
    if (t == "sint64") return FieldType::Sint64;
    if (t == "fixed32") return FieldType::Fixed32;
    if (t == "fixed64") return FieldType::Fixed64;
    if (t == "sfixed32") return FieldType::Sfixed32;
    if (t == "sfixed64") return FieldType::Sfixed64;
    if (t == "bool") return FieldType::Bool;
    if (t == "string") return FieldType::String;
    return FieldType::Bytes;  // "bytes"
}

class Parser {
public:
    Parser(const std::vector<Token>& tokens, std::string filename, ProtoFile& file)
        : toks_(tokens), filename_(std::move(filename)), file_(file) {}

    void run() {
        while (!at_end()) {
            parse_top_level();
        }
    }

private:
    const std::vector<Token>& toks_;
    std::string filename_;
    ProtoFile& file_;
    std::size_t pos_ = 0;

    [[nodiscard]] const Token& peek() const { return toks_[pos_]; }
    [[nodiscard]] bool at_end() const { return peek().kind == TokKind::End; }
    const Token& advance() { return toks_[pos_++]; }

    [[noreturn]] void error(const std::string& msg) const {
        throw std::runtime_error(filename_ + ":" + std::to_string(peek().line) +
                                 ": parser: " + msg + " (got '" + peek().text + "')");
    }

    bool is_ident(const char* s) const {
        return peek().kind == TokKind::Identifier && peek().text == s;
    }
    bool is_symbol(const char c) const {
        return peek().kind == TokKind::Symbol && peek().text.size() == 1 && peek().text[0] == c;
    }

    void expect_symbol(const char c) {
        if (!is_symbol(c))
            error(std::string("expected '") + c + "'");
        advance();
    }
    std::string expect_identifier() {
        if (peek().kind != TokKind::Identifier)
            error("expected identifier");
        return advance().text;
    }

    // ---- top level --------------------------------------------------------

    void parse_top_level() {
        if (is_ident("syntax")) {
            advance();
            expect_symbol('=');
            if (peek().kind != TokKind::String)
                error("expected syntax string");
            if (const std::string syntax = advance().text; syntax != "proto3" && syntax != "proto2")
                error("unsupported syntax '" + syntax + "'");
            expect_symbol(';');
            return;
        }
        if (is_ident("package")) {
            advance();
            file_.package = expect_identifier();
            expect_symbol(';');
            return;
        }
        if (is_ident("import")) {
            advance();
            if (peek().kind != TokKind::String)
                error("expected import path string");
            advance();
            expect_symbol(';');
            return;
        }
        if (is_ident("option")) {
            // File-level option: discard.
            skip_until_semicolon();
            return;
        }
        if (is_ident("enum")) {
            parse_enum();
            return;
        }
        if (is_ident("message")) {
            parse_message();
            return;
        }
        if (is_ident("service")) {
            // service Name { ... } — skip entirely (RPCs are out of scope).
            advance();
            expect_identifier();
            skip_braced_block();
            return;
        }
        if (is_ident("extend")) {
            // extend google.protobuf.X { ... } — option definitions, skip.
            advance();
            expect_identifier();
            skip_braced_block();
            return;
        }
        error("unexpected top-level token");
    }

    // ---- enum -------------------------------------------------------------

    void parse_enum() {
        advance();  // 'enum'
        ProtoEnum e;
        e.name = expect_identifier();
        expect_symbol('{');
        while (!is_symbol('}')) {
            if (at_end())
                error("unterminated enum");
            if (is_ident("option") || is_ident("reserved")) {
                skip_until_semicolon();
                continue;
            }
            ProtoEnumValue v;
            v.name = expect_identifier();
            expect_symbol('=');
            v.number = parse_integer();
            skip_field_options_if_present();
            expect_symbol(';');
            e.values.push_back(std::move(v));
        }
        expect_symbol('}');
        file_.enums.push_back(std::move(e));
    }

    // ---- message ----------------------------------------------------------

    void parse_message() {
        advance();  // 'message'
        ProtoMessageDef m;
        m.name = expect_identifier();
        expect_symbol('{');
        while (!is_symbol('}')) {
            if (at_end())
                error("unterminated message");
            parse_message_member(m);
        }
        expect_symbol('}');
        file_.messages.push_back(std::move(m));
    }

    void parse_message_member(ProtoMessageDef& m) {
        if (is_ident("option")) {
            parse_message_option(m);
            return;
        }
        if (is_ident("reserved")) {
            skip_until_semicolon();
            return;
        }
        // Reject everything outside the supported subset, loudly.
        if (is_ident("oneof") || is_ident("map") || is_ident("group") || is_ident("message") ||
            is_ident("enum") || is_ident("extend") || is_ident("extensions")) {
            error("unsupported construct in message body");
        }

        // Field: ["repeated" | "optional"] type name = number [opts] ;
        ProtoField f;
        if (is_ident("repeated")) {
            advance();
            f.repeated = true;
        } else if (is_ident("optional") || is_ident("required")) {
            // proto2 leftovers — treat as singular.
            advance();
        }
        f.type_name = expect_identifier();
        f.name = expect_identifier();
        expect_symbol('=');
        f.number = static_cast<int>(parse_integer());
        parse_field_options(f);
        expect_symbol(';');
        m.fields.push_back(std::move(f));
    }

    void parse_message_option(ProtoMessageDef& m) {
        advance();  // 'option'
        // option (name) = value;  or  option name = value;
        std::string opt_name;
        if (is_symbol('(')) {
            advance();
            opt_name = expect_identifier();
            expect_symbol(')');
        } else {
            opt_name = expect_identifier();
        }
        expect_symbol('=');

        if (opt_name == "id") {
            m.id = static_cast<std::uint32_t>(parse_integer());
        } else if (opt_name == "base_class") {
            if (peek().kind != TokKind::String)
                error("expected string for (base_class)");
            m.base_class = advance().text;
        } else {
            // Discard the value of any other message option.
            skip_option_value();
        }
        expect_symbol(';');
    }

    void parse_field_options(ProtoField& f) {
        if (!is_symbol('['))
            return;
        advance();  // '['
        while (!is_symbol(']')) {
            if (at_end())
                error("unterminated field options");
            bool parenthesized = false;
            std::string name;
            if (is_symbol('(')) {
                advance();
                name = expect_identifier();
                expect_symbol(')');
                parenthesized = true;
            } else {
                name = expect_identifier();
            }
            expect_symbol('=');
            if (!parenthesized && name == "packed") {
                f.packed_explicit = true;
                f.packed_value = parse_bool();
            } else {
                skip_option_value();
            }
            if (is_symbol(','))
                advance();
        }
        expect_symbol(']');
    }

    void skip_field_options_if_present() {
        if (is_symbol('[')) {
            int depth = 0;
            do {
                if (is_symbol('['))
                    ++depth;
                else if (is_symbol(']'))
                    --depth;
                advance();
            } while (depth > 0 && !at_end());
        }
    }

    // ---- helpers ----------------------------------------------------------

    std::int64_t parse_integer() {
        if (peek().kind != TokKind::Number)
            error("expected integer");
        const std::string text = advance().text;
        try {
            return static_cast<std::int64_t>(std::stoll(text, nullptr, 0));
        } catch (...) {
            error("invalid integer '" + text + "'");
        }
    }

    bool parse_bool() {
        if (is_ident("true")) {
            advance();
            return true;
        }
        if (is_ident("false")) {
            advance();
            return false;
        }
        error("expected true/false");
    }

    // Skip a single option value (string / number / bool / identifier / aggregate).
    void skip_option_value() {
        if (is_symbol('{')) {
            skip_braced_block();
            return;
        }
        if (peek().kind == TokKind::String || peek().kind == TokKind::Number ||
            peek().kind == TokKind::Identifier) {
            advance();
            return;
        }
        error("expected option value");
    }

    void skip_until_semicolon() {
        while (!is_symbol(';')) {
            if (at_end())
                error("expected ';'");
            if (is_symbol('{') || is_symbol('[')) {
                skip_balanced();
                continue;
            }
            advance();
        }
        advance();  // ';'
    }

    // Skip a {...} block; assumes the next token is '{'.
    void skip_braced_block() {
        expect_symbol('{');
        int depth = 1;
        while (depth > 0) {
            if (at_end())
                error("unterminated block");
            if (is_symbol('{'))
                ++depth;
            else if (is_symbol('}'))
                --depth;
            advance();
        }
    }

    // Skip a balanced {} or [] starting at the current token.
    void skip_balanced() {
        const char open = peek().text[0];
        const char close = open == '{' ? '}' : ']';
        int depth = 0;
        do {
            if (at_end())
                error("unterminated block");
            if (is_symbol(open))
                ++depth;
            else if (is_symbol(close))
                --depth;
            advance();
        } while (depth > 0);
    }
};

}  // namespace

void parse_into(const std::vector<Token>& tokens, const std::string& filename, ProtoFile& file) {
    Parser(tokens, filename, file).run();
}

void resolve_types(ProtoFile& file) {
    std::unordered_set<std::string> enums;
    std::unordered_set<std::string> messages;
    for (const auto& [name, values] : file.enums)
        enums.insert(name);
    for (const auto& m : file.messages)
        messages.insert(m.name);

    auto simple_name = [](const std::string& raw) {
        std::string t = raw;
        if (!t.empty() && t[0] == '.')
            t.erase(0, 1);
        if (const auto dot = t.rfind('.'); dot != std::string::npos)
            t = t.substr(dot + 1);
        return t;
    };

    for (auto& m : file.messages) {
        for (auto& f : m.fields) {
            if (is_scalar_type(f.type_name)) {
                f.type = scalar_field_type(f.type_name);
                continue;
            }
            if (const std::string name = simple_name(f.type_name); enums.count(name) != 0U) {
                f.type = FieldType::Enum;
                f.type_name = name;
            } else if (messages.count(name) != 0U) {
                f.type = FieldType::Message;
                f.type_name = name;
            } else {
                throw std::runtime_error("unknown field type '" + f.type_name + "' in message " +
                                         m.name + " field " + f.name);
            }
        }
    }
}

bool is_scalar_type(const std::string& t) {
    for (const char* s : kScalarTypes)
        if (t == s)
            return true;
    return false;
}

int wire_type_of(const ProtoField& f) {
    switch (f.type) {
    case FieldType::Double:
    case FieldType::Fixed64:
    case FieldType::Sfixed64:
        return 1;
    case FieldType::Float:
    case FieldType::Fixed32:
    case FieldType::Sfixed32:
        return 5;
    case FieldType::String:
    case FieldType::Bytes:
    case FieldType::Message:
        return 2;
    default:
        return 0;  // varint: int/uint/sint/bool/enum
    }
}

bool is_packed(const ProtoField& f) {
    if (!f.repeated)
        return false;
    // Only numeric scalar / enum fields can be packed.
    if (f.type == FieldType::String || f.type == FieldType::Bytes || f.type == FieldType::Message)
        return false;
    if (f.packed_explicit)
        return f.packed_value;
    return true;  // proto3 default: packed
}

}  // namespace protogen
