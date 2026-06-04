#pragma once

/// @file
/// @brief Tiny glob matcher used for watch patterns over "<domain>.<object_id>".

#include <string>
#include <vector>

namespace cli {

/// Match `text` against a glob `pattern` where '*' matches any run of chars.
/// As a convenience, a trailing ".*" also matches the prefix itself, so
/// "light.foo.*" matches "light.foo".
bool glob_match(const std::string& pattern, const std::string& text);

/// True if `text` matches any pattern (empty list ⇒ matches everything).
bool glob_match_any(const std::vector<std::string>& patterns, const std::string& text);

}  // namespace cli
