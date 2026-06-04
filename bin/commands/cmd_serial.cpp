#include <esphome/api/exception.hpp>
#include <esphome/api/json.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>
#include <esphome/api/sync_client.hpp>

#include "commands/actions/entity_actions.hpp"
#include "commands/commands.hpp"
#include "commands/serial_server.hpp"
#include "net/connection.hpp"
#include "net/loop.hpp"

#include <chrono>
#include <iostream>
#include <string>

namespace cli {
namespace {

using esphome::api::SerialPort;
using esphome::api::SerialProxyConfig;
using esphome::api::SerialProxyParity;
using esphome::api::SyncClient;

SerialProxyParity parse_parity(const std::string& s) {
    if (s == "none" || s == "n")
        return SerialProxyParity::None;
    if (s == "even" || s == "e")
        return SerialProxyParity::Even;
    if (s == "odd" || s == "o")
        return SerialProxyParity::Odd;
    throw std::invalid_argument("unknown parity: " + s);
}

SerialPort resolve_port(SyncClient& client, const std::string& token) {
    try {
        return client.serial_proxy(parse_uint(token));
    } catch (const std::invalid_argument&) {
        return client.serial_proxy(token);  // by advertised name
    }
}

/// Build a SerialProxyConfig from "k=v" tokens starting at `begin`.
SerialProxyConfig parse_config(const std::uint32_t instance,
                               const std::vector<std::string>& args,
                               const std::size_t begin) {
    SerialProxyConfig cfg;
    cfg.instance = instance;
    for (std::size_t i = begin; i < args.size(); ++i) {
        const auto pos = args[i].find('=');
        if (pos == std::string::npos)
            throw std::invalid_argument("expected key=value, got: " + args[i]);
        const std::string key = args[i].substr(0, pos);
        const std::string val = args[i].substr(pos + 1);
        if (key == "baud" || key == "baudrate")
            cfg.baudrate = parse_uint(val);
        else if (key == "parity")
            cfg.parity = parse_parity(val);
        else if (key == "stop_bits" || key == "stop")
            cfg.stop_bits = parse_uint(val);
        else if (key == "data_size" || key == "data_bits" || key == "data")
            cfg.data_size = parse_uint(val);
        else if (key == "flow_control" || key == "flow")
            cfg.flow_control = parse_bool(val);
        else
            throw std::invalid_argument("unknown config field: " + key);
    }
    return cfg;
}

int serial_listen(const CliContext& ctx, SyncClient& client, const std::vector<std::string>& args) {
    const SerialPort port = resolve_port(client, args.size() > 1 ? args[1] : std::string("0"));
    const OutputWriter& out = ctx.out;
    client.serial().on_data(port.index(), [&out](const esphome::api::SerialProxyData& d) {
        out.stream_line(d.data, d);
    });
    port.subscribe();
    run_until_signal(client);
    return 0;
}

int serial_configure(const CliContext& ctx,
                     SyncClient& client,
                     const std::vector<std::string>& args) {
    std::size_t cfg_begin = 1;
    std::uint32_t instance = 0;
    if (args.size() > 1 && args[1].find('=') == std::string::npos) {
        instance = parse_uint(args[1]);
        cfg_begin = 2;
    }
    const SerialProxyConfig cfg = parse_config(instance, args, cfg_begin);
    client.serial().configure(cfg);
    try {
        client.pump_until([] { return false; }, std::chrono::milliseconds(500));
    } catch (const esphome::api::TimeoutError&) {}
    ctx.out.emit(cfg, [&](std::ostream& os) {
        os << "configured instance " << cfg.instance << " baud=" << cfg.baudrate << "\n";
    });
    return 0;
}

int serial_write(const CliContext& ctx, SyncClient& client, const std::vector<std::string>& args) {
    if (args.size() < 3)
        throw std::invalid_argument("write requires <index> <data>");
    const SerialPort port = resolve_port(client, args[1]);
    port.write(args[2]);
    try {
        client.pump_until([] { return false; }, std::chrono::milliseconds(500));
    } catch (const esphome::api::TimeoutError&) {}
    ctx.out.emit({{"instance", port.index()}, {"written", args[2].size()}},
                 [&](std::ostream& os) { os << "wrote " << args[2].size() << " byte(s)\n"; });
    return 0;
}

int serial_pins(const CliContext& ctx, SyncClient& client, const std::vector<std::string>& args) {
    if (args.size() < 2)
        throw std::invalid_argument("pins requires get|set");
    const std::string& op = args[1];

    if (op == "get") {
        const SerialPort port = resolve_port(client, args.size() > 2 ? args[2] : std::string("0"));
        bool done = false;
        esphome::api::SerialProxyModemPins pins;
        port.get_modem_pins([&](const esphome::api::SerialProxyModemPins& p) {
            pins = p;
            done = true;
        });
        try {
            client.pump_until([&] { return done; },
                              std::chrono::milliseconds(ctx.request_timeout_ms()));
        } catch (const esphome::api::TimeoutError&) {}
        if (!done) {
            std::cerr << "error: timed out reading modem pins\n";
            return 5;
        }
        ctx.out.emit(pins, [&](std::ostream& os) {
            os << "rts=" << (pins.lines.rts ? 1 : 0) << " dtr=" << (pins.lines.dtr ? 1 : 0) << "\n";
        });
        return 0;
    }

    if (op == "set") {
        std::uint32_t instance = 0;
        esphome::api::SerialProxyLineStates lines;
        for (std::size_t i = 2; i < args.size(); ++i) {
            const auto pos = args[i].find('=');
            if (pos == std::string::npos) {
                instance = parse_uint(args[i]);
                continue;
            }
            const std::string key = args[i].substr(0, pos);
            const std::string val = args[i].substr(pos + 1);
            if (key == "rts")
                lines.rts = parse_bool(val);
            else if (key == "dtr")
                lines.dtr = parse_bool(val);
            else
                throw std::invalid_argument("unknown pin: " + key);
        }
        client.serial_proxy(instance).set_modem_pins(lines);
        try {
            client.pump_until([] { return false; }, std::chrono::milliseconds(300));
        } catch (const esphome::api::TimeoutError&) {}
        ctx.out.emit({{"rts", lines.rts}, {"dtr", lines.dtr}}, [&](std::ostream& os) {
            os << "set rts=" << (lines.rts ? 1 : 0) << " dtr=" << (lines.dtr ? 1 : 0) << "\n";
        });
        return 0;
    }

    throw std::invalid_argument("pins expects get|set");
}

}  // namespace

int cmd_serial(CliContext& ctx, const std::string& host, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "usage: esphome-cli <host> serial-proxy "
                     "<listen|configure|pins|write|server> [...]\n";
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
        if (sub == "listen")
            return serial_listen(ctx, client, args);
        if (sub == "configure")
            return serial_configure(ctx, client, args);
        if (sub == "write")
            return serial_write(ctx, client, args);
        if (sub == "pins")
            return serial_pins(ctx, client, args);
        if (sub == "server") {
            std::uint32_t instance = 0;
            std::size_t cfg_begin = 1;
            if (args.size() > 1 && args[1].rfind("--", 0) != 0 &&
                args[1].find('=') == std::string::npos) {
                instance = parse_uint(args[1]);
                cfg_begin = 2;
            }
            SerialProxyConfig initial;
            initial.instance = instance;
            initial.baudrate = 115200;
            return serial_server(
                ctx,
                client,
                instance,
                initial,
                std::vector<std::string>(args.begin() + static_cast<std::ptrdiff_t>(cfg_begin),
                                         args.end()));
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        client.disconnect();
        return 2;
    }

    std::cerr << "error: unknown serial-proxy subcommand '" << sub << "'\n";
    client.disconnect();
    return 2;
}

}  // namespace cli
