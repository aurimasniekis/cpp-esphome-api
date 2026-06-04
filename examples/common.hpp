#pragma once

/// @file
/// @brief Shared command-line parsing for the examples. Every example accepts:
///   <host> [port] [--key|-k <base64-psk>] [--name|-n <device-name>] [extra...]
/// The optional PSK enables Noise encryption; `--name` verifies the device name.

#include <esphome/api/api.hpp>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace example {

struct Cli {
    esphome::api::ClientOptions options;
    std::vector<std::string> positionals;  ///< non-flag args after host (port, seconds, ...)
    bool ok = false;
};

inline std::uint16_t to_port(const std::string& text) {
    return static_cast<std::uint16_t>(std::atoi(text.c_str()));
}

/// Parse argv into ClientOptions. Prints usage and returns ok=false on --help or
/// a missing host. `usage` is the example-specific positional summary.
inline Cli parse(int argc, char** argv, const std::string& usage, const std::string& client_info) {
    Cli cli;
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        std::cout << "usage: " << usage << " [--key|-k <base64-psk>] [--name|-n <device-name>]\n";
        return cli;
    }
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        if ((a == "--key" || a == "-k") && i + 1 < args.size()) {
            cli.options.connection.noise_psk = args[++i];
        } else if ((a == "--name" || a == "-n") && i + 1 < args.size()) {
            cli.options.connection.expected_name = args[++i];
        } else if (cli.options.host.empty()) {
            cli.options.host = a;
        } else {
            cli.positionals.push_back(a);
        }
    }
    if (!cli.positionals.empty()) {
        cli.options.port = to_port(cli.positionals[0]);
    }
    cli.options.connection.client_info = client_info;
    cli.ok = !cli.options.host.empty();
    return cli;
}

}  // namespace example
