#pragma once

/// @file
/// @brief PTY bridge: expose a remote UART as a local pseudo-terminal so any
///        serial tool can talk to it. POSIX-only (stubbed elsewhere).

#include <esphome/api/subsystems/serial_proxy.hpp>
#include <esphome/api/sync_client.hpp>

#include "app/cli_context.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace cli {

/// Run the PTY bridge for an already-connected client. `args` are the tokens
/// after "server" (--link, --baud, --instance, --parity, --rts, --dtr).
int serial_server(const CliContext& ctx,
                  esphome::api::SyncClient& client,
                  std::uint32_t instance,
                  const esphome::api::SerialProxyConfig& initial,
                  const std::vector<std::string>& args);

}  // namespace cli
