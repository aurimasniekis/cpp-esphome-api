#include <esphome/api/exception.hpp>
#include <esphome/api/json.hpp>
#include <esphome/api/sync_client.hpp>

#include "commands/actions/entity_actions.hpp"
#include "commands/commands.hpp"
#include "net/connection.hpp"
#include "net/loop.hpp"

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace cli {
namespace {

constexpr std::string_view hex_digits = "0123456789abcdef";

std::string addr_to_mac(const std::uint64_t addr) {
    std::string out;
    for (int shift = 40; shift >= 0; shift -= 8) {
        const auto b = static_cast<unsigned>((addr >> shift) & 0xFFU);
        out.push_back(static_cast<char>(std::toupper(hex_digits[b >> 4])));
        out.push_back(static_cast<char>(std::toupper(hex_digits[b & 0x0FU])));
        if (shift != 0)
            out.push_back(':');
    }
    return out;
}

std::string hex_to_bytes(const std::string& in) {
    std::string clean;
    for (const char c : in)
        if (std::isxdigit(static_cast<unsigned char>(c)) != 0)
            clean.push_back(c);
    if (clean.size() % 2 != 0)
        throw std::invalid_argument("hex data must have an even number of digits");
    std::string out;
    out.reserve(clean.size() / 2);
    for (std::size_t i = 0; i < clean.size(); i += 2)
        out.push_back(
            static_cast<char>(std::strtol(clean.substr(i, 2).c_str(), nullptr, 16) & 0xFF));
    return out;
}

std::string bytes_to_hex(const std::string& data) {
    std::string out;
    out.reserve(data.size() * 2);
    for (const char c : data) {
        const auto b = static_cast<unsigned char>(c);
        out.push_back(hex_digits[b >> 4]);
        out.push_back(hex_digits[b & 0x0FU]);
    }
    return out;
}

int bt_scan(const CliContext& ctx,
            esphome::api::SyncClient& client,
            const std::vector<std::string>& a) {
    int seconds = 0;
    for (std::size_t i = 1; i < a.size(); ++i) {
        if (a[i] == "active")
            client.bluetooth().set_scanner_mode(esphome::api::BluetoothScannerMode::Active);
        else
            seconds = parse_int(a[i]);
    }

    const OutputWriter& out = ctx.out;
    client.bluetooth().subscribe_advertisements([&out](const esphome::api::BleAdvertisement& adv) {
        nlohmann::json doc = adv;
        doc["mac"] = addr_to_mac(adv.address);
        out.stream_line(addr_to_mac(adv.address) + "  rssi=" + std::to_string(adv.rssi) + "  " +
                            (adv.name.empty() ? "(no name)" : adv.name),
                        doc);
    });

    if (seconds > 0) {
        try {
            client.pump_until([] { return false; }, std::chrono::seconds(seconds));
        } catch (const esphome::api::TimeoutError&) {}
        client.disconnect();
    } else {
        run_until_signal(client);
    }
    return 0;
}

int bt_connect(const CliContext& ctx, esphome::api::SyncClient& client, const std::uint64_t addr) {
    bool done = false;
    esphome::api::BleConnection conn;
    client.bluetooth().on_connection([&](const esphome::api::BleConnection& c) {
        if (c.address == addr) {
            conn = c;
            done = true;
        }
    });
    client.bluetooth().request_connection(addr);
    try {
        client.pump_until([&] { return done; },
                          std::chrono::milliseconds(ctx.request_timeout_ms()));
    } catch (const esphome::api::TimeoutError&) {}
    if (!done) {
        std::cerr << "error: timed out waiting for connection to " << addr_to_mac(addr) << "\n";
        return 5;
    }
    ctx.out.emit(conn, [&](std::ostream& os) {
        os << addr_to_mac(addr) << "  " << (conn.connected ? "connected" : "disconnected")
           << "  mtu=" << conn.mtu << "\n";
    });
    return conn.connected ? 0 : 5;
}

int bt_read(const CliContext& ctx,
            esphome::api::SyncClient& client,
            const std::uint64_t addr,
            const std::uint32_t handle) {
    bool done = false;
    esphome::api::BleGattReadResult result;
    client.bluetooth().read(addr, handle, [&](const esphome::api::BleGattReadResult& r) {
        result = r;
        done = true;
    });
    try {
        client.pump_until([&] { return done; },
                          std::chrono::milliseconds(ctx.request_timeout_ms()));
    } catch (const esphome::api::TimeoutError&) {}
    if (!done || !result.ok) {
        std::cerr << "error: GATT read failed (error " << result.error << ")\n";
        return 5;
    }
    nlohmann::json doc = result;
    doc["hex"] = bytes_to_hex(result.data);
    ctx.out.emit(doc, [&](std::ostream& os) { os << bytes_to_hex(result.data) << "\n"; });
    return 0;
}

int bt_write(const CliContext& ctx,
             esphome::api::SyncClient& client,
             const std::uint64_t addr,
             const std::uint32_t handle,
             const std::string& data,
             const bool response) {
    bool done = false;
    esphome::api::BleGattWriteResult result;
    client.bluetooth().write(
        addr, handle, data, response, [&](const esphome::api::BleGattWriteResult& r) {
            result = r;
            done = true;
        });
    try {
        client.pump_until([&] { return done; },
                          std::chrono::milliseconds(ctx.request_timeout_ms()));
    } catch (const esphome::api::TimeoutError&) {}
    if (!done || !result.ok) {
        std::cerr << "error: GATT write failed (error " << result.error << ")\n";
        return 5;
    }
    ctx.out.emit(result, [&](std::ostream& os) { os << "ok\n"; });
    return 0;
}

int bt_services(const CliContext& ctx, esphome::api::SyncClient& client, const std::uint64_t addr) {
    bool done = false;
    esphome::api::BleGattServicesResult result;
    client.bluetooth().get_services(addr, [&](const esphome::api::BleGattServicesResult& r) {
        result = r;
        done = true;
    });
    try {
        client.pump_until([&] { return done; },
                          std::chrono::milliseconds(ctx.request_timeout_ms() * 2));
    } catch (const esphome::api::TimeoutError&) {}
    if (!done || !result.ok) {
        std::cerr << "error: GATT service discovery failed (error " << result.error << ")\n";
        return 5;
    }
    ctx.out.emit(result, [&](std::ostream& os) {
        os << addr_to_mac(addr) << "  " << result.services.size() << " service(s)\n";
        for (const auto& svc : result.services)
            os << "  service handle=" << svc.handle
               << "  characteristics=" << svc.characteristics.size() << "\n";
    });
    return 0;
}

}  // namespace

int cmd_bt(CliContext& ctx, const std::string& host, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "usage: esphome-cli <host> bt <scan|connect|read|write|services> [...]\n";
        return 2;
    }
    const std::string& sub = args.front();

    const esphome::api::ClientOptions opt = resolve_options(ctx, host);
    esphome::api::SyncClient client(opt);
    client.set_request_timeout(std::chrono::milliseconds(ctx.request_timeout_ms()));
    try {
        client.connect();
    } catch (const std::exception& e) {
        return report_connect_error(ctx, host, e);
    }
    persist_connection(ctx, host, opt, {});

    try {
        if (sub == "scan")
            return bt_scan(ctx, client, args);
        if (sub == "connect") {
            if (args.size() < 2)
                throw std::invalid_argument("connect requires <addr>");
            return bt_connect(ctx, client, parse_ble_addr(args[1]));
        }
        if (sub == "read") {
            if (args.size() < 3)
                throw std::invalid_argument("read requires <addr> <handle>");
            return bt_read(ctx, client, parse_ble_addr(args[1]), parse_uint(args[2]));
        }
        if (sub == "write") {
            if (args.size() < 4)
                throw std::invalid_argument("write requires <addr> <handle> <hex>");
            bool response = true;
            for (std::size_t i = 4; i < args.size(); ++i)
                if (args[i] == "no-response" || args[i] == "noresp")
                    response = false;
            return bt_write(ctx,
                            client,
                            parse_ble_addr(args[1]),
                            parse_uint(args[2]),
                            hex_to_bytes(args[3]),
                            response);
        }
        if (sub == "services") {
            if (args.size() < 2)
                throw std::invalid_argument("services requires <addr>");
            return bt_services(ctx, client, parse_ble_addr(args[1]));
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        client.disconnect();
        return 2;
    }

    std::cerr << "error: unknown bt subcommand '" << sub << "'\n";
    client.disconnect();
    return 2;
}

}  // namespace cli
