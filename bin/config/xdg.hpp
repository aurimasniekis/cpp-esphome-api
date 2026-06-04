#pragma once

/// @file
/// @brief XDG base-directory resolution for the CLI's config location.

#include <filesystem>

namespace cli {

/// The esphome-cli config directory: $XDG_CONFIG_HOME/esphome-cli if set, else
/// ~/.config/esphome-cli (same on macOS, by design — not ~/Library).
std::filesystem::path config_dir();

/// The default config file path: config_dir() / "config.json".
std::filesystem::path default_config_path();

}  // namespace cli
