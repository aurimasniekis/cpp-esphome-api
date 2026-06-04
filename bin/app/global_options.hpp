#pragma once

/// @file
/// @brief Attach global options to a CLI11 app + build the CLI's own logger.

#include "app/cli_context.hpp"
#include <CLI/CLI.hpp>

#include <memory>
#include <string>

namespace spdlog {
class logger;
}

namespace cli {

/// The esphome-cli version string (mirrors the library version).
const char* cli_version();

/// The HelloRequest client_info reported to devices ("esphome-cli/<version>").
std::string client_info();

/// Register every global option (with env-var bindings) on `app`, writing into
/// `g`. Safe to call on both the global and per-host apps.
void add_global_options(CLI::App& app, GlobalOptions& g);

/// Build a stderr logger at the given level name (trace/debug/info/warn/error/
/// critical/off; "warn"/"error" accepted). Unknown names default to warn.
std::shared_ptr<spdlog::logger> make_logger(const std::string& level);

}  // namespace cli
