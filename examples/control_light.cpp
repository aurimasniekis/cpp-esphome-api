// control_light — discover the first light entity and toggle it, using the
// object-oriented entity API.
//
//   ./esphome_api_control_light <host> [port] [--key <base64-psk>]

#include "common.hpp"

#include <chrono>
#include <iostream>

int main(int argc, char** argv) {
    auto cli = example::parse(
        argc, argv, "esphome_api_control_light <host> [port]", "esphome-api-client/control_light");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }

    // Pump the loop for a fixed duration (states keep flowing meanwhile).
    auto wait = [](esphome::api::SyncClient& c, std::chrono::milliseconds d) {
        try {
            c.pump_until([] { return false; }, d);
        } catch (const esphome::api::TimeoutError&) {}
    };

    try {
        esphome::api::SyncClient client(cli.options);
        client.connect();  // auto-enumerates entities + subscribes to states

        auto lights = client.entities().lights();
        if (lights.empty()) {
            std::cout << "No light entity found on " << client.server_hello().name << "\n";
            client.disconnect();
            return 0;
        }

        auto light = lights[0];
        std::cout << "Controlling light '" << light.name() << "' (" << light.object_id() << ")\n";

        std::cout << "Turning on at full brightness...\n";
        light.set_brightness(1.0F);
        wait(client, std::chrono::seconds(2));

        std::cout << "Turning off...\n";
        light.turn_off();
        wait(client, std::chrono::seconds(2));

        client.disconnect();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
