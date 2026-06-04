// bluetooth_scan — subscribe to BLE advertisements via the device's proxy.
//
//   ./esphome_api_bluetooth_scan <host> [port] [seconds] [--key <base64-psk>]

#include "common.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    auto cli = example::parse(argc,
                              argv,
                              "esphome_api_bluetooth_scan <host> [port] [seconds]",
                              "esphome-api-client/bluetooth_scan");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }
    const int seconds = cli.positionals.size() >= 2 ? std::atoi(cli.positionals[1].c_str()) : 15;

    try {
        esphome::api::SyncClient client(cli.options);
        client.connect();
        std::cout << "Connected to " << client.server_hello().name << ". Scanning for " << seconds
                  << "s ...\n";

        client.bluetooth().subscribe_advertisements([](const esphome::api::BleAdvertisement& adv) {
            std::printf("  %012llX  rssi=%-4d  %s\n",
                        static_cast<unsigned long long>(adv.address),
                        static_cast<int>(adv.rssi),
                        adv.name.empty() ? "(no name)" : adv.name.c_str());
        });

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
