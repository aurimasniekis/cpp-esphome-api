#include "app/help.hpp"

#include "commands/actions/entity_actions.hpp"
#include "commands/commands.hpp"
#include "commands/domains.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace cli {

void print_root_help(std::ostream& os) {
    os << "esphome-cli — discover, inspect and control ESPHome devices\n\n"
          "usage:\n"
          "  esphome-cli [globals] <command> [args]\n"
          "  esphome-cli [globals] <host> <subcommand> [args]\n\n"
          "global commands:\n"
          "  scan [seconds]                 Discover devices on the LAN (mDNS)\n"
          "  config <list|get|set|forget|path> [...]   Manage saved devices/defaults\n"
          "  devices                        List saved devices\n"
          "  help [tokens...]               Show contextual help\n\n"
          "host subcommands:\n"
          "  info [--entities]              Device info (and entities/states)\n"
          "  list                           Every entity with info/state/actions\n"
          "  <domain> <object-id> <action> [values...]   Control an entity\n"
          "  logs [--level LEVEL]           Stream device logs\n"
          "  watch [patterns...]            Stream entity state changes\n"
          "  bt <scan|connect|read|write|services> [...]   Bluetooth proxy\n"
          "  serial-proxy <listen|configure|pins|write|server> [...]   Serial (UART) proxy\n\n"
          "domains:\n  ";
    bool first = true;
    for (const auto& [command, type] : domains()) {
        os << (first ? "" : " ") << command;
        first = false;
    }
    os << "\n\n"
          "global options:\n"
          "  --output text|json|yaml        Output format\n"
          "  --log-level LEVEL              CLI log verbosity\n"
          "  --config PATH                  Config file location\n"
          "  -p, --port N                   API TCP port (default 6053)\n"
          "  -k, --key PSK                  Base64 Noise PSK (enables encryption)\n"
          "  -n, --name NAME                Expected device name\n"
          "  --save-keys / --no-save-keys   Persist encryption keys to config\n"
          "  --timeout MS                   Per-request timeout\n\n"
          "environment variables (flags override env, which overrides config):\n"
          "  ESPHOME_CLI_HOST  ESPHOME_CLI_KEY  ESPHOME_CLI_CONFIG  ESPHOME_CLI_PORT\n"
          "  ESPHOME_CLI_NAME  ESPHOME_CLI_LOG_LEVEL  ESPHOME_CLI_OUTPUT\n"
          "  ESPHOME_CLI_SAVE_KEYS  ESPHOME_CLI_TIMEOUT\n";
}

namespace {

void print_domain_help(std::ostream& os,
                       const std::string& domain,
                       const esphome::api::EntityType type) {
    os << "usage:\n"
       << "  esphome-cli <host> " << domain << "                       list " << domain
       << " entities\n"
       << "  esphome-cli <host> " << domain << " <object-id>           list its actions\n"
       << "  esphome-cli <host> " << domain << " <object-id> <action> [values...]\n";
    os << "actions:";
    for (const std::string& a : actions_for(type))
        os << " " << a;
    os << " info state\n";  // info/state are available on every entity
}

void print_bt_help(std::ostream& os) {
    os << "usage: esphome-cli <host> bt <subcommand> [args]\n"
          "  scan [seconds]                 Stream BLE advertisements\n"
          "  connect <addr>                 Connect to a BLE device\n"
          "  read <addr> <handle>           Read a GATT characteristic\n"
          "  write <addr> <handle> <hex>    Write a GATT characteristic\n"
          "  services <addr>                Discover the GATT service table\n"
          "addresses are MACs (AA:BB:CC:DD:EE:FF) or hex/decimal.\n";
}

void print_serial_help(std::ostream& os) {
    os << "usage: esphome-cli <host> serial-proxy <subcommand> [args]\n"
          "  listen [index]                 Dump incoming serial bytes\n"
          "  configure [index] [k=v...]     Configure UART (baud=, parity=, ...)\n"
          "  pins <get|set rts=.. dtr=..>   Read/set modem control pins\n"
          "  write <index> <data>           Write data to the UART\n"
          "  server [--link PATH] [--baud N]  Bridge a local PTY to the remote UART\n";
}

}  // namespace

int cmd_help(const CliContext& ctx, const std::vector<std::string>& tokens) {
    (void)ctx;
    if (tokens.empty()) {
        print_root_help(std::cout);
        return 0;
    }

    const std::string& topic = tokens.front();
    if (const esphome::api::EntityType type = domain_to_type(topic);
        type != esphome::api::EntityType::Unknown) {
        print_domain_help(std::cout, topic, type);
        return 0;
    }
    if (topic == "bt") {
        print_bt_help(std::cout);
        return 0;
    }
    if (topic == "serial-proxy" || topic == "serial") {
        print_serial_help(std::cout);
        return 0;
    }
    if (topic == "scan") {
        std::cout << "usage: esphome-cli scan [seconds]\n"
                     "Discover ESPHome devices on the local network via mDNS.\n";
        return 0;
    }
    if (topic == "config") {
        std::cout << "usage: esphome-cli config <list|get|set|forget|path> [...]\n"
                     "  list                 Show defaults and saved devices\n"
                     "  get <host>           Show one saved device\n"
                     "  set <host> k=v ...   Set device fields "
                     "(address/port/psk/name/expected_name/timeout_ms)\n"
                     "  set defaults k=v ... Set defaults "
                     "(output/log_level/scan_timeout_ms/timeout_ms/save_keys)\n"
                     "  forget <host>        Remove a saved device\n"
                     "  path                 Print the config file path\n";
        return 0;
    }
    if (topic == "devices") {
        std::cout << "usage: esphome-cli devices\nList devices saved in the config.\n";
        return 0;
    }
    if (topic == "info") {
        std::cout << "usage: esphome-cli <host> info [--entities]\n"
                     "Show device info; with --entities also list entities and states.\n";
        return 0;
    }
    if (topic == "list") {
        std::cout << "usage: esphome-cli <host> list\n"
                     "List every entity, each with its info, state and available actions.\n";
        return 0;
    }
    if (topic == "logs") {
        std::cout << "usage: esphome-cli <host> logs [--level LEVEL]\n"
                     "Stream the device log; Ctrl-C to stop.\n";
        return 0;
    }
    if (topic == "watch") {
        std::cout << "usage: esphome-cli <host> watch [patterns...]\n"
                     "Stream entity state changes. Patterns are globs over "
                     "'<domain>.<object_id>' (e.g. 'light.*'); no args watches everything.\n";
        return 0;
    }

    print_root_help(std::cout);
    return 0;
}

}  // namespace cli
