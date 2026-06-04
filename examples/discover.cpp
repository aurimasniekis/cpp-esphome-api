// discover — scan the local network for ESPHome devices via mDNS.
//
//   ./esphome_api_discover [seconds]

#include <esphome/api/api.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--help") {
        std::cout << "usage: esphome_api_discover [seconds]\n";
        return 0;
    }
    const int seconds = argc >= 2 ? std::atoi(argv[1]) : 3;

    std::cout << "Scanning for ESPHome devices for " << seconds << "s...\n";
    const auto devices = esphome::api::Discovery::scan(std::chrono::seconds(seconds));

    if (devices.empty()) {
        std::cout << "No devices found.\n";
        return 0;
    }
    for (const auto& d : devices) {
        std::cout << "\n" << d.name << "  (" << d.friendly_name << ")\n";
        std::cout << "  host:       " << d.connect_host() << ":" << d.port << "\n";
        std::cout << "  mac:        " << d.mac << "\n";
        std::cout << "  version:    " << d.version << "  platform=" << d.platform
                  << " board=" << d.board << "\n";
        std::cout << "  encryption: "
                  << (d.requires_encryption   ? "required"
                      : d.supports_encryption ? "supported"
                                              : "none")
                  << "\n";
        if (!d.project_name.empty()) {
            std::cout << "  project:    " << d.project_name << " " << d.project_version << "\n";
        }
    }
    std::cout << "\n" << devices.size() << " device(s) found.\n";

    // The discovered host can be fed straight into a client:
    //   esphome::api::ClientOptions opt;
    //   opt.host = devices[0].connect_host();
    //   opt.port = devices[0].port;
    return 0;
}
