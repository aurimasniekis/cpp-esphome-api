#pragma once

/// @file
/// @brief Persistent CLI configuration: per-device records + defaults.

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace cli {

/// One saved device. The map key in Config::devices is the user-facing host
/// alias (typically the hostname/IP originally used).
struct DeviceRecord {
    std::string name;               ///< Friendly/device name (informational).
    std::string address;            ///< Host or IP to connect to.
    std::uint16_t port = 6053;      ///< API port.
    std::string psk;                ///< Base64 Noise PSK (may be empty / stripped).
    std::string expected_name;      ///< Optional expected device name guard.
    std::optional<int> timeout_ms;  ///< Optional per-device request timeout (ms).
};

/// The esphome-cli config file (`~/.config/esphome-cli/config.json`).
class Config {
public:
    /// Load from `path`; returns a default-initialised Config (remembering the
    /// path) if the file is missing or unreadable.
    static Config load(const std::filesystem::path& path);

    /// Persist to the configured path, creating the directory (0700) and file
    /// (0600). Throws std::exception on I/O failure.
    void save();

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

    // --- Defaults ------------------------------------------------------------
    std::string default_output = "text";
    std::string default_log_level = "warn";
    int scan_timeout_ms = 2000;
    int request_timeout_ms = 30000;
    bool save_keys = true;

    // --- Devices -------------------------------------------------------------
    std::map<std::string, DeviceRecord> devices;

    /// Find a device by config key, then by address, then by name. Returns
    /// nullptr if none match.
    [[nodiscard]] const DeviceRecord* find(const std::string& host) const;

    /// Insert or update the record for `host`. When `save_keys_effective` is
    /// false the PSK is stripped before storing. The first time a non-empty PSK
    /// is persisted a one-time plaintext-storage notice is printed to stderr.
    void remember(const std::string& host, DeviceRecord record, bool save_keys_effective);

    /// Remove the record for `host` (by key/address/name). Returns true if a
    /// record was removed.
    bool forget(const std::string& host);

private:
    std::filesystem::path path_;
    bool plaintext_warned_ = false;

    /// Resolve `host` to a stored map key (matching by key/address/name).
    [[nodiscard]] const std::string* resolve_key(const std::string& host) const;
};

}  // namespace cli
