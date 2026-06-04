// logs — stream the device's logs to stdout.
//
//   ./esphome_api_logs <host> [port] [seconds] [--key <base64-psk>]

#include "common.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    auto cli = example::parse(
        argc, argv, "esphome_api_logs <host> [port] [seconds]", "esphome-api-client/logs");
    if (!cli.ok) {
        return argc < 2 ? 1 : 0;
    }
    const int seconds = cli.positionals.size() >= 2 ? std::atoi(cli.positionals[1].c_str()) : 30;

    try {
        esphome::api::SyncClient client(cli.options);
        client.connect();

        client.logs().subscribe(
            esphome::api::LogLevel::VeryVerbose,
            [](const esphome::api::LogEntry& entry) { std::cout << entry.message << "\n"; });

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
