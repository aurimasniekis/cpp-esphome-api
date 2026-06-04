#pragma once

/// @file
/// @brief Local-network discovery of ESPHome devices via mDNS / DNS-SD
///        (service `_esphomelib._tcp`). No Asio in this header.

#include <esphome/api/bytes.hpp>

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace esphome::api {

/// An ESPHome device found on the local network.
struct DiscoveredDevice {
    /// Service instance name (the device's friendly/host name).
    std::string name;
    /// SRV target host, e.g. "living-room.local".
    std::string hostname;
    /// Resolved IP address (from the A/AAAA record); empty if not provided.
    std::string address;
    /// API TCP port (default 6053).
    std::uint16_t port = 6053;

    // Common TXT metadata (also available verbatim in `properties`).
    std::string mac;
    std::string version;
    std::string friendly_name;
    std::string platform;
    std::string board;
    std::string network;
    std::string project_name;
    std::string project_version;

    /// True when the device advertises that Noise encryption is required
    /// (TXT `api_encryption`). `supports_encryption` is true when encryption is
    /// at least supported (TXT `api_encryption` or `api_encryption_supported`).
    bool requires_encryption = false;
    bool supports_encryption = false;

    /// Every TXT record, as key → value.
    std::map<std::string, std::string> properties;

    /// Best host to connect to: the resolved IP if known, else the hostname.
    [[nodiscard]] std::string connect_host() const {
        return address.empty() ? hostname : address;
    }
};

/// Discovers ESPHome devices via multicast DNS.
class Discovery {
public:
    /// Blocking scan: multicasts a DNS-SD query and collects responses for
    /// `timeout`. Returns one entry per discovered device (deduplicated by name).
    /// Requires network access to 224.0.0.251:5353.
    static std::vector<DiscoveredDevice>
    scan(std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
};

/// Parse raw mDNS response packets into devices. Exposed for testing and reuse;
/// `Discovery::scan` calls this on the packets it collects.
std::vector<DiscoveredDevice> parse_mdns_packets(const std::vector<ByteBuffer>& packets);

}  // namespace esphome::api
