#include "net/connection.hpp"

#include <esphome/api/exception.hpp>

#include "app/global_options.hpp"

#include <chrono>
#include <iostream>

#include <spdlog/spdlog.h>

namespace cli {

esphome::api::ClientOptions resolve_options(const CliContext& ctx, const std::string& host) {
    esphome::api::ClientOptions opt;
    std::string address = host;

    if (const DeviceRecord* rec = ctx.config.find(host); rec != nullptr) {
        if (!rec->address.empty())
            address = rec->address;
        opt.port = rec->port;
        opt.connection.noise_psk = rec->psk;
        opt.connection.expected_name = rec->expected_name;
    }

    if (ctx.globals.port)
        opt.port = *ctx.globals.port;
    if (!ctx.globals.key.empty())
        opt.connection.noise_psk = ctx.globals.key;
    if (!ctx.globals.name.empty())
        opt.connection.expected_name = ctx.globals.name;

    opt.host = address;
    opt.connection.client_info = client_info();
    opt.subscribe_on_connect = false;
    opt.connection.connect_timeout = std::chrono::milliseconds(ctx.request_timeout_ms());
    return opt;
}

int resolve_timeout_ms(const GlobalOptions& globals,
                       const Config& config,
                       const std::string& host) {
    if (globals.timeout_ms)
        return *globals.timeout_ms;
    if (!host.empty())
        if (const DeviceRecord* rec = config.find(host); rec != nullptr && rec->timeout_ms)
            return *rec->timeout_ms;
    return config.request_timeout_ms;
}

int report_connect_error(const CliContext& ctx, const std::string& host, const std::exception& e) {
    namespace api = esphome::api;
    int code = 5;
    std::string hint;

    if (dynamic_cast<const api::EncryptionMismatchError*>(&e) != nullptr) {
        // The message already names the corrective action; no generic hint.
        code = 3;
    } else if (dynamic_cast<const api::EncryptionError*>(&e) != nullptr ||
               dynamic_cast<const api::HandshakeError*>(&e) != nullptr ||
               dynamic_cast<const api::AuthenticationError*>(&e) != nullptr) {
        code = 3;
        hint = " (check the encryption key with --key / -k, or the expected name)";
    } else if (dynamic_cast<const api::ConnectionError*>(&e) != nullptr ||
               dynamic_cast<const api::TimeoutError*>(&e) != nullptr) {
        code = 4;
    }

    ctx.log->debug("connect to {} failed: {}", host, e.what());
    std::cerr << "error: failed to connect to " << host << ": " << e.what() << hint << "\n";
    return code;
}

void persist_connection(CliContext& ctx,
                        const std::string& host,
                        const esphome::api::ClientOptions& opt,
                        const std::string& device_name) {
    const bool save = ctx.save_keys();

    DeviceRecord desired;
    if (const DeviceRecord* cur = ctx.config.find(host); cur != nullptr)
        desired = *cur;
    desired.address = opt.host;
    desired.port = opt.port;
    desired.expected_name = opt.connection.expected_name;
    if (!device_name.empty())
        desired.name = device_name;
    desired.psk = save ? opt.connection.noise_psk : std::string{};

    const DeviceRecord* cur = ctx.config.find(host);
    const bool changed = cur == nullptr || cur->address != desired.address ||
                         cur->port != desired.port || cur->expected_name != desired.expected_name ||
                         cur->name != desired.name || cur->psk != desired.psk;
    if (!changed)
        return;

    ctx.config.remember(host, desired, save);
    try {
        ctx.config.save();
    } catch (const std::exception& ex) {
        ctx.log->warn("could not save config: {}", ex.what());
    }
}

}  // namespace cli
