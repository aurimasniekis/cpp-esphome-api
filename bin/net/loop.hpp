#pragma once

/// @file
/// @brief Drive a connected client until a stop signal arrives.

#include <esphome/api/sync_client.hpp>

#include <chrono>

namespace cli {

/// Run the client's event loop in bounded slices until SIGINT/SIGTERM is seen
/// (or the link drops), then disconnect. Installs the signal handlers.
void run_until_signal(esphome::api::SyncClient& client,
                      std::chrono::milliseconds slice = std::chrono::milliseconds(200));

}  // namespace cli
