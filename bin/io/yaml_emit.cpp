#include "io/yaml_emit.hpp"

#include <array>
#include <cctype>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace cli {
namespace {

/// Whether a string scalar must be quoted to round-trip as YAML.
bool needs_quote(const std::string& s) {
    if (s.empty())
        return true;

    static constexpr std::array<std::string_view, 19> reserved = {"true",
                                                                  "false",
                                                                  "null",
                                                                  "yes",
                                                                  "no",
                                                                  "on",
                                                                  "off",
                                                                  "~",
                                                                  "True",
                                                                  "False",
                                                                  "Null",
                                                                  "Yes",
                                                                  "No",
                                                                  "On",
                                                                  "Off",
                                                                  "TRUE",
                                                                  "FALSE",
                                                                  "NULL",
                                                                  "YES"};
    for (const std::string_view r : reserved)
        if (s == r)
            return true;

    const auto front = static_cast<unsigned char>(s.front());
    if (const auto back = static_cast<unsigned char>(s.back());
        std::isspace(front) != 0 || std::isspace(back) != 0)
        return true;

    static constexpr std::string_view special = "-?:,[]{}#&*!|>'\"%@`";
    if (special.find(s.front()) != std::string_view::npos)
        return true;

    if (s.find(": ") != std::string::npos || s.find(" #") != std::string::npos)
        return true;
    if (s.back() == ':')
        return true;
    for (const char c : s)
        if (c == '\n' || c == '\t')
            return true;

    // Numeric-looking strings must stay quoted to remain strings.
    char* end = nullptr;
    const double value = std::strtod(s.c_str(), &end);
    static_cast<void>(value);
    return end != nullptr && *end == '\0';
}

/// Render a scalar (or empty container) JSON value as inline YAML text.
std::string inline_scalar(const nlohmann::json& v) {
    if (v.is_null())
        return "null";
    if (v.is_boolean())
        return v.get<bool>() ? "true" : "false";
    if (v.is_number())
        return v.dump();
    if (v.is_string()) {
        const auto s = v.get<std::string>();
        return needs_quote(s) ? nlohmann::json(s).dump() : s;
    }
    if (v.is_array())
        return "[]";  // only called for empty arrays
    return "{}";      // only called for empty objects
}

bool is_inline(const nlohmann::json& v) {
    return v.is_primitive() || (v.is_array() && v.empty()) || (v.is_object() && v.empty());
}

std::string format_key(const std::string& key) {
    return needs_quote(key) ? nlohmann::json(key).dump() : key;
}

void write_block(const nlohmann::json& node, int indent, std::ostream& os) {
    const std::string pad(static_cast<std::size_t>(indent) * 2, ' ');
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            os << pad << format_key(it.key()) << ":";
            if (is_inline(it.value())) {
                os << " " << inline_scalar(it.value()) << "\n";
            } else {
                os << "\n";
                write_block(it.value(), indent + 1, os);
            }
        }
    } else {  // array
        const std::string child_pad(static_cast<std::size_t>(indent + 1) * 2, ' ');
        for (const auto& item : node) {
            if (is_inline(item)) {
                os << pad << "- " << inline_scalar(item) << "\n";
                continue;
            }
            // Render the nested block, then splice "- " onto its first line so
            // the item reads as "- key: value" (same indentation width).
            std::ostringstream tmp;
            write_block(item, indent + 1, tmp);
            std::string block = tmp.str();
            block.replace(0, child_pad.size(), pad + "- ");
            os << block;
        }
    }
}

std::string strip_trailing_newline(std::string s) {
    while (!s.empty() && s.back() == '\n')
        s.pop_back();
    return s;
}

}  // namespace

std::string to_yaml(const nlohmann::json& doc) {
    if (is_inline(doc))
        return inline_scalar(doc);
    std::ostringstream os;
    write_block(doc, 0, os);
    return strip_trailing_newline(os.str());
}

std::string to_yaml_list_item(const nlohmann::json& value) {
    if (is_inline(value))
        return "- " + inline_scalar(value);
    std::ostringstream os;
    write_block(value, 1, os);
    std::string body = strip_trailing_newline(os.str());
    // Replace the leading two-space pad of the first line with "- ".
    if (body.size() >= 2)
        body.replace(0, 2, "- ");
    return body;
}

}  // namespace cli
