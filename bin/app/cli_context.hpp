#pragma once

/// @file
/// @brief Shared state threaded through every command: resolved globals, the
///        config, and the output writer.

#include "config/config.hpp"
#include "io/output.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace spdlog {
class logger;
}

namespace cli {

/// Raw global options as parsed from flags/env. Empty / nullopt means "not
/// provided" so config records and built-in defaults can take over in
/// resolve_options() and friends. Precedence: flag > env > config > default.
struct GlobalOptions {
    std::string config_path;            ///< --config / ESPHOME_CLI_CONFIG
    std::string log_level;              ///< --log-level / ESPHOME_CLI_LOG_LEVEL
    std::string output;                 ///< --output / ESPHOME_CLI_OUTPUT
    std::optional<std::uint16_t> port;  ///< --port / ESPHOME_CLI_PORT
    std::string key;                    ///< --key,-k / ESPHOME_CLI_KEY
    std::string name;                   ///< --name,-n / ESPHOME_CLI_NAME
    std::optional<bool> save_keys;      ///< --save-keys/--no-save-keys / ESPHOME_CLI_SAVE_KEYS
    std::optional<int> timeout_ms;      ///< --timeout / ESPHOME_CLI_TIMEOUT
};

/// Everything a command needs, assembled by the dispatcher after parsing. The
/// references point at dispatcher-owned objects that outlive every command call.
struct CliContext {
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const GlobalOptions& globals;
    Config& config;
    const OutputWriter& out;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::shared_ptr<spdlog::logger> log;

    /// Effective per-request timeout in milliseconds, resolved by the dispatcher
    /// (flag/env > per-device config > config default > built-in).
    int timeout_ms = 30000;

    /// Effective key-persistence setting (flag/env override, else config).
    [[nodiscard]] bool save_keys() const {
        return globals.save_keys.value_or(config.save_keys);
    }

    /// Effective per-request timeout in milliseconds.
    [[nodiscard]] int request_timeout_ms() const {
        return timeout_ms;
    }
};

}  // namespace cli
