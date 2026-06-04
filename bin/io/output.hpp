#pragma once

/// @file
/// @brief Format-aware output writer (text / JSON / YAML).

#include <nlohmann/json.hpp>

#include <functional>
#include <ostream>
#include <string>

namespace cli {

enum class Format { Text, Json, Yaml };

/// Parse "text"/"json"/"yaml" into a Format (defaults to Text on anything else).
Format parse_format(const std::string& name);

/// Writes structured results in the selected format. For one-shot documents use
/// emit(); for incremental streams (logs, scans, watch) use stream_line().
class OutputWriter {
public:
    explicit OutputWriter(const Format format) : format_(format) {}

    [[nodiscard]] Format format() const noexcept {
        return format_;
    }

    /// Emit a complete document. In text mode `render_text` is invoked to write a
    /// human-friendly rendering; in JSON/YAML mode `doc` is serialized.
    void emit(const nlohmann::json& doc,
              const std::function<void(std::ostream&)>& render_text) const;

    /// Emit one record of a continuing stream. Text mode prints `text`; JSON mode
    /// prints `obj` as NDJSON (one compact object per line); YAML mode prints it
    /// as a "- " sequence item. Output is flushed so streams stay live.
    void stream_line(const std::string& text, const nlohmann::json& obj) const;

private:
    Format format_;
};

}  // namespace cli
