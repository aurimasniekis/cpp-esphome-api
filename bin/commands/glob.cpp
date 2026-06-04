#include "commands/glob.hpp"

#include <algorithm>

namespace cli {
namespace {

/// Classic '*' wildcard match with backtracking ('?' matches one char).
bool wildcard(const std::string& pat, const std::string& s) {
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t star = std::string::npos;
    std::size_t mark = 0;
    while (t < s.size()) {
        if (p < pat.size() && (pat[p] == '?' || pat[p] == s[t])) {
            ++p;
            ++t;
        } else if (p < pat.size() && pat[p] == '*') {
            star = p++;
            mark = t;
        } else if (star != std::string::npos) {
            p = star + 1;
            t = ++mark;
        } else {
            return false;
        }
    }
    while (p < pat.size() && pat[p] == '*')
        ++p;
    return p == pat.size();
}

}  // namespace

bool glob_match(const std::string& pattern, const std::string& text) {
    if (pattern == "*")
        return true;
    if (pattern.size() >= 2 && pattern.compare(pattern.size() - 2, 2, ".*") == 0 &&
        text == pattern.substr(0, pattern.size() - 2))
        return true;
    return wildcard(pattern, text);
}

bool glob_match_any(const std::vector<std::string>& patterns, const std::string& text) {
    if (patterns.empty())
        return true;
    return std::any_of(patterns.begin(), patterns.end(), [&](const std::string& p) {
        return glob_match(p, text);
    });
}

}  // namespace cli
