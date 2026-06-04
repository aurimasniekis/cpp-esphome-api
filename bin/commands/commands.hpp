#pragma once

/// @file
/// @brief Entry points for every CLI command. Each returns a process exit code.

#include "app/cli_context.hpp"

#include <string>
#include <vector>

namespace cli {

// --- Global (host-less) commands -------------------------------------------
int cmd_scan(const CliContext& ctx, int timeout_ms);
int cmd_config(CliContext& ctx, const std::string& action, const std::vector<std::string>& args);
int cmd_devices(const CliContext& ctx);
int cmd_help(const CliContext& ctx, const std::vector<std::string>& tokens);

// --- Host-scoped commands ---------------------------------------------------
int cmd_info(CliContext& ctx, const std::string& host, bool entities);
int cmd_list(CliContext& ctx, const std::string& host);
int cmd_entity(CliContext& ctx,
               const std::string& host,
               const std::string& domain,
               const std::string& object_id,
               const std::string& action,
               const std::vector<std::string>& values);

// --- Phase 2: streaming subsystems -----------------------------------------
int cmd_logs(CliContext& ctx, const std::string& host, const std::string& level);
int cmd_watch(CliContext& ctx, const std::string& host, const std::vector<std::string>& patterns);
int cmd_bt(CliContext& ctx, const std::string& host, const std::vector<std::string>& args);

// --- Phase 3: serial proxy --------------------------------------------------
int cmd_serial(CliContext& ctx, const std::string& host, const std::vector<std::string>& args);

}  // namespace cli
