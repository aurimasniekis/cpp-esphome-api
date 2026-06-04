// subscribe_states — connect and print live state updates per entity.
//
//   ./esphome_api_subscribe_states <host> [port] [seconds] [--key <base64-psk>]

#include "common.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    auto cli = example::parse(argc,
                              argv,
                              "esphome_api_subscribe_states <host> [port] [seconds]",
                              "esphome-api-client/subscribe_states");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }
    const int seconds = cli.positionals.size() >= 2 ? std::atoi(cli.positionals[1].c_str()) : 30;

    try {
        esphome::api::SyncClient client(cli.options);
        client.connect();  // auto-enumerates entities + subscribes to states

        client.store().on_state([&](esphome::api::EntityType type, std::uint32_t key) {
            const auto* e = client.store().find(key);
            std::cout << "[" << esphome::api::entity_type_name(type) << "] "
                      << (e ? e->name : std::string{}) << " updated\n";
        });

        std::cout << "Subscribed. Streaming state changes for " << seconds << "s...\n";
        try {
            client.pump_until([] { return false; }, std::chrono::seconds(seconds));
        } catch (const esphome::api::TimeoutError&) {}
        client.disconnect();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
