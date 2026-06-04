#pragma once

/// @file
/// @brief Typed view of the device's DeviceInfoResponse.

#include <esphome/api/model/enums.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace esphome::api {

namespace proto {
class DeviceInfoResponse;
}  // namespace proto

/// One serial-proxy port advertised by the device. Its instance index (used by
/// SerialProxy) is its position in DeviceInfo::serial_proxies.
struct SerialProxyPortInfo {
    std::string name;
    SerialProxyPortType port_type = SerialProxyPortType::Ttl;
};

/// Ergonomic, copyable snapshot of a device's identity and capabilities,
/// converted from the raw DeviceInfoResponse.
struct DeviceInfo {
    std::string name;
    std::string friendly_name;
    std::string mac_address;
    std::string bluetooth_mac_address;
    std::string esphome_version;
    std::string compilation_time;
    std::string model;
    std::string manufacturer;
    std::string project_name;
    std::string project_version;
    std::string suggested_area;
    bool has_deep_sleep = false;
    std::uint32_t webserver_port = 0;
    bool api_encryption_supported = false;

    std::uint32_t bluetooth_proxy_feature_flags = 0;
    std::uint32_t voice_assistant_feature_flags = 0;
    std::uint32_t zwave_proxy_feature_flags = 0;
    std::uint32_t zwave_home_id = 0;

    /// Serial-proxy ports exposed by the device (index = instance).
    std::vector<SerialProxyPortInfo> serial_proxies;

    /// Build a DeviceInfo from a decoded DeviceInfoResponse.
    static DeviceInfo from_proto(const proto::DeviceInfoResponse& msg);
};

}  // namespace esphome::api
