#pragma once

/// @file
/// @brief Top-level argument classification and command dispatch.

namespace cli {

/// Parse argv, classify global-vs-host mode, and run the selected command.
/// Returns the process exit code.
int run(int argc, char** argv);

}  // namespace cli
