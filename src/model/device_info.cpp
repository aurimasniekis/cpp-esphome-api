#include <esphome/api/model/device_info.hpp>

#include "api.pb.h"

namespace esphome::api {

DeviceInfo DeviceInfo::from_proto(const proto::DeviceInfoResponse& msg) {
    DeviceInfo info;
    info.name = msg.name();
    info.friendly_name = msg.friendly_name();
    info.mac_address = msg.mac_address();
    info.bluetooth_mac_address = msg.bluetooth_mac_address();
    info.esphome_version = msg.esphome_version();
    info.compilation_time = msg.compilation_time();
    info.model = msg.model();
    info.manufacturer = msg.manufacturer();
    info.project_name = msg.project_name();
    info.project_version = msg.project_version();
    info.suggested_area = msg.suggested_area();
    info.has_deep_sleep = msg.has_deep_sleep();
    info.webserver_port = msg.webserver_port();
    info.api_encryption_supported = msg.api_encryption_supported();
    info.bluetooth_proxy_feature_flags = msg.bluetooth_proxy_feature_flags();
    info.voice_assistant_feature_flags = msg.voice_assistant_feature_flags();
    info.zwave_proxy_feature_flags = msg.zwave_proxy_feature_flags();
    info.zwave_home_id = msg.zwave_home_id();
    for (int i = 0; i < msg.serial_proxies_size(); ++i) {
        const auto& sp = msg.serial_proxies(i);
        info.serial_proxies.push_back(
            {sp.name(), static_cast<SerialProxyPortType>(sp.port_type())});
    }
    return info;
}

}  // namespace esphome::api
