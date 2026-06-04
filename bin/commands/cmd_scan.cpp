#include <esphome/api/discovery.hpp>
#include <esphome/api/json.hpp>

#include "commands/commands.hpp"

#include <chrono>
#include <ostream>

#include <spdlog/spdlog.h>

namespace cli {

int cmd_scan(const CliContext& ctx, const int timeout_ms) {
    ctx.log->debug("scanning for {} ms", timeout_ms);
    const auto devices = esphome::api::Discovery::scan(std::chrono::milliseconds(timeout_ms));

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& d : devices)
        arr.push_back(d);

    ctx.out.emit(arr, [&](std::ostream& os) {
        if (devices.empty()) {
            os << "No devices found.\n";
            return;
        }
        for (const auto& d : devices) {
            os << d.name;
            if (!d.friendly_name.empty())
                os << "  (" << d.friendly_name << ")";
            os << "\n";
            os << "  host:       " << d.connect_host() << ":" << d.port << "\n";
            if (!d.mac.empty())
                os << "  mac:        " << d.mac << "\n";
            os << "  version:    " << d.version << "\n";
            if (d.supports_encryption)
                os << "  encryption: " << (d.requires_encryption ? "required" : "supported")
                   << "\n";
            else
                os << "  encryption: " << (d.requires_encryption ? "required" : "none") << "\n";
        }
        os << "\n" << devices.size() << " device(s) found.\n";
    });
    return 0;
}

}  // namespace cli
