#pragma once

/// @file
/// @brief SIGINT/SIGTERM handling for streaming commands. The handler only sets
///        an atomic flag; the event loop polls it between bounded run slices.

namespace cli {

/// Install handlers for SIGINT and SIGTERM (idempotent).
void install_signal_handlers();

/// True once a stop signal has been received.
bool stop_requested();

}  // namespace cli
