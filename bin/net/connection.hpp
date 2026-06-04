#pragma once

/// @file
/// @brief Build ClientOptions from globals+config and map connect failures to
///        exit codes / persist successful connections.

#include <esphome/api/client_options.hpp>

#include "app/cli_context.hpp"

#include <exception>
#include <string>

namespace cli {

/// Layer CLI flags > config record > defaults into a ClientOptions for `host`.
esphome::api::ClientOptions resolve_options(const CliContext& ctx, const std::string& host);

/// Effective request timeout (ms) for `host`: flag/env > per-device config >
/// config default. `host` may be empty (global commands) to skip the per-device
/// lookup.
int resolve_timeout_ms(const GlobalOptions& globals, const Config& config, const std::string& host);

/// Print + log a connection failure and return the matching exit code:
///   3 = auth/handshake/encryption, 4 = connection/timeout, 5 = protocol/other.
int report_connect_error(const CliContext& ctx, const std::string& host, const std::exception& e);

/// Persist a successful connection's coordinates (address/port/name/psk) to the
/// config when they changed. Honors the effective save_keys setting.
void persist_connection(CliContext& ctx,
                        const std::string& host,
                        const esphome::api::ClientOptions& opt,
                        const std::string& device_name);

}  // namespace cli
