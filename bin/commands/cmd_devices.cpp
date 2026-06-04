#include "commands/commands.hpp"
#include <nlohmann/json.hpp>

#include <ostream>

namespace cli {

int cmd_devices(const CliContext& ctx) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& [host, rec] : ctx.config.devices) {
        arr.push_back({{"host", host},
                       {"name", rec.name},
                       {"address", rec.address},
                       {"port", rec.port},
                       {"has_key", !rec.psk.empty()},
                       {"expected_name", rec.expected_name}});
    }

    ctx.out.emit(arr, [&](std::ostream& os) {
        if (ctx.config.devices.empty()) {
            os << "No saved devices. Connect to one (or run 'config set') to save it.\n";
            return;
        }
        for (const auto& [host, rec] : ctx.config.devices) {
            os << host;
            if (!rec.name.empty() && rec.name != host)
                os << "  (" << rec.name << ")";
            os << "\n";
            os << "  address:   " << (rec.address.empty() ? host : rec.address) << ":" << rec.port
               << "\n";
            os << "  key:       " << (rec.psk.empty() ? "no" : "yes (stored)") << "\n";
            if (!rec.expected_name.empty())
                os << "  expect:    " << rec.expected_name << "\n";
        }
    });
    return 0;
}

}  // namespace cli
