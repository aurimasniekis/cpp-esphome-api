// hello — connect to a device, print its identity, disconnect.
//
//   ./esphome_api_hello <host> [port] [--key <base64-psk>]

#include "common.hpp"

#include <iostream>

int main(int argc, char** argv) {
    auto cli =
        example::parse(argc, argv, "esphome_api_hello <host> [port]", "esphome-api-client/hello");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }

    try {
        esphome::api::SyncClient client(cli.options);
        std::cout << "Connecting to " << cli.options.host << ":" << cli.options.port
                  << (cli.options.connection.noise_psk.empty() ? " (plaintext)" : " (encrypted)")
                  << " ...\n";
        client.connect();

        const auto& hello = client.server_hello();
        std::cout << "Connected. API " << hello.api_version_major << "." << hello.api_version_minor
                  << " — " << hello.name << " (" << hello.server_info << ")\n";

        const auto info = client.device_info();
        std::cout << "Device:        " << info.name << " (" << info.friendly_name << ")\n";
        std::cout << "Model:         " << info.model << " by " << info.manufacturer << "\n";
        std::cout << "ESPHome:       " << info.esphome_version << "\n";
        std::cout << "MAC:           " << info.mac_address << "\n";
        std::cout << "Project:       " << info.project_name << " " << info.project_version << "\n";
        std::cout << "Encryption:    " << (info.api_encryption_supported ? "supported" : "no")
                  << "\n";

        client.disconnect();
        std::cout << "Disconnected.\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
