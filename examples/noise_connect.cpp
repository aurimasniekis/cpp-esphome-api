// noise_connect — connect to an encrypted device using its base64 PSK.
//
//   ./esphome_api_noise_connect <host> --key <base64-psk> [port]
//
// (Every other example also accepts --key; this one simply requires it.)

#include "common.hpp"

#include <iostream>

int main(int argc, char** argv) {
    auto cli = example::parse(
        argc, argv, "esphome_api_noise_connect <host> [port]", "esphome-api-client/noise_connect");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }
    if (cli.options.connection.noise_psk.empty()) {
        std::cerr << "error: this example requires --key <base64-psk>\n";
        return 1;
    }

    try {
        esphome::api::SyncClient client(cli.options);
        std::cout << "Connecting (encrypted) to " << cli.options.host << ":" << cli.options.port
                  << " ...\n";
        client.connect();

        const auto& hello = client.server_hello();
        std::cout << "Secure session established with " << hello.name << " (API "
                  << hello.api_version_major << "." << hello.api_version_minor << ")\n";

        const auto info = client.device_info();
        std::cout << "Device: " << info.name << " — " << info.model << " / " << info.esphome_version
                  << "\n";

        client.disconnect();
        std::cout << "Disconnected.\n";
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
