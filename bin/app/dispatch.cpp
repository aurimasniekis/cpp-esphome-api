#include "app/dispatch.hpp"

#include "app/cli_context.hpp"
#include "app/global_options.hpp"
#include "app/help.hpp"
#include "commands/commands.hpp"
#include "commands/domains.hpp"
#include "config/xdg.hpp"
#include "io/output.hpp"
#include "net/connection.hpp"
#include <CLI/CLI.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cli {
namespace {

/// Owns the per-invocation config + output + logger that CliContext references.
struct Session {
    Config config;
    std::unique_ptr<OutputWriter> out;
    std::shared_ptr<spdlog::logger> log;
};

Session make_session(const GlobalOptions& g) {
    Session s;
    const std::filesystem::path path =
        g.config_path.empty() ? default_config_path() : std::filesystem::path(g.config_path);
    s.config = Config::load(path);

    const std::string output = !g.output.empty() ? g.output : s.config.default_output;
    const std::string level = !g.log_level.empty() ? g.log_level : s.config.default_log_level;
    s.out = std::make_unique<OutputWriter>(parse_format(output));
    s.log = make_logger(level);
    return s;
}

/// Parse `args` (no program name) with `app`. Returns an exit code if parsing
/// finished early (error / --help / --version), or nullopt to continue.
std::optional<int> parse_app(CLI::App& app, const std::vector<std::string>& args) {
    std::vector<const char*> argv;
    argv.reserve(args.size() + 1);
    argv.push_back("esphome-cli");
    for (const std::string& a : args)
        argv.push_back(a.c_str());
    try {
        app.parse(static_cast<int>(argv.size()), argv.data());
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }
    return std::nullopt;
}

/// Index of the first positional token, accounting for value-taking globals.
/// Returns args.size() if there is no positional.
std::size_t find_first_positional(const std::vector<std::string>& args) {
    static const std::set<std::string> value_opts = {"--config",
                                                     "--log-level",
                                                     "--output",
                                                     "--port",
                                                     "-p",
                                                     "--key",
                                                     "-k",
                                                     "--name",
                                                     "-n",
                                                     "--timeout"};
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (const std::string& a = args[i]; a.size() > 1 && a[0] == '-') {
            if (a.find('=') != std::string::npos)
                continue;
            if (value_opts.count(a) != 0)
                ++i;  // skip this option's value
            continue;
        }
        return i;
    }
    return args.size();
}

int run_global(const std::vector<std::string>& args) {
    CLI::App app{"esphome-cli — discover, inspect and control ESPHome devices", "esphome-cli"};
    app.set_version_flag("--version", std::string("esphome-cli ") + cli_version());
    app.require_subcommand(0, 1);

    GlobalOptions g;
    add_global_options(app, g);

    auto* scan = app.add_subcommand("scan", "Discover ESPHome devices via mDNS");
    scan->fallthrough();
    std::optional<int> scan_seconds;
    scan->add_option("seconds", scan_seconds, "Scan duration in seconds");

    auto* config = app.add_subcommand("config", "Manage saved devices and defaults");
    config->fallthrough();
    std::string config_action;
    std::vector<std::string> config_args;
    config->add_option("action", config_action, "list|get|set|forget|path");
    config->add_option("args", config_args, "action arguments");

    auto* devices = app.add_subcommand("devices", "List saved devices");
    devices->fallthrough();

    auto* help = app.add_subcommand("help", "Show contextual help");
    help->fallthrough();
    std::vector<std::string> help_tokens;
    help->add_option("tokens", help_tokens, "Help topic tokens");

    if (const std::optional<int> rc = parse_app(app, args); rc)
        return *rc;

    Session s = make_session(g);
    CliContext ctx{g, s.config, *s.out, s.log};
    ctx.timeout_ms = resolve_timeout_ms(g, s.config, {});

    if (help->parsed())
        return cmd_help(ctx, help_tokens);
    if (scan->parsed()) {
        const int ms = scan_seconds ? *scan_seconds * 1000 : s.config.scan_timeout_ms;
        return cmd_scan(ctx, ms);
    }
    if (config->parsed())
        return cmd_config(ctx, config_action, config_args);
    if (devices->parsed())
        return cmd_devices(ctx);

    print_root_help(std::cout);
    return 0;
}

int run_host(const std::string& host, const std::vector<std::string>& args) {
    CLI::App app{"Control ESPHome device " + host, "esphome-cli"};
    app.require_subcommand(1);

    GlobalOptions g;
    add_global_options(app, g);

    auto* info = app.add_subcommand("info", "Show device info");
    info->fallthrough();
    bool info_entities = false;
    info->add_flag("--entities", info_entities, "Include entities and their states");

    auto* list = app.add_subcommand("list", "List every entity with info/state/actions");
    list->fallthrough();

    std::string ent;
    std::string act;
    std::vector<std::string> vals;
    std::map<CLI::App*, esphome::api::EntityType> domain_apps;
    for (const DomainEntry& d : domains()) {
        auto* sc = app.add_subcommand(d.command,
                                      std::string("Inspect/control ") + d.command + " entities");
        sc->fallthrough();
        // All optional: "<domain>" lists entities, "<domain> <id>" lists actions.
        sc->add_option("object_id", ent, "Entity object id (omit to list the domain)");
        sc->add_option("action", act, "Action verb (omit to list actions; state/info to inspect)");
        sc->add_option("values", vals, "Action values");
        domain_apps[sc] = d.type;
    }

    auto* logs = app.add_subcommand("logs", "Stream device logs");
    logs->fallthrough();
    std::string log_level_opt;
    logs->add_option("--level", log_level_opt, "Minimum log level");

    auto* watch = app.add_subcommand("watch", "Stream entity state changes");
    watch->fallthrough();
    std::vector<std::string> watch_patterns;
    watch->add_option("patterns", watch_patterns, "Glob patterns over <domain>.<object_id>");

    auto* bt = app.add_subcommand("bt", "Bluetooth proxy");
    bt->prefix_command();

    auto* serial = app.add_subcommand("serial-proxy", "Serial (UART) proxy");
    serial->prefix_command();

    if (const std::optional<int> rc = parse_app(app, args); rc)
        return *rc;

    Session s = make_session(g);
    CliContext ctx{g, s.config, *s.out, s.log};
    ctx.timeout_ms = resolve_timeout_ms(g, s.config, host);

    if (info->parsed())
        return cmd_info(ctx, host, info_entities);
    if (list->parsed())
        return cmd_list(ctx, host);
    for (const auto& [sc, type] : domain_apps)
        if (sc->parsed())
            return cmd_entity(ctx, host, type_to_domain(type), ent, act, vals);
    if (logs->parsed())
        return cmd_logs(ctx, host, log_level_opt);
    if (watch->parsed())
        return cmd_watch(ctx, host, watch_patterns);
    if (bt->parsed())
        return cmd_bt(ctx, host, bt->remaining());
    if (serial->parsed())
        return cmd_serial(ctx, host, serial->remaining());
    return 0;
}

}  // namespace

int run(const int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);

    static const std::set<std::string> global_cmds = {"scan", "config", "devices", "help"};
    std::set<std::string> host_cmds = {"info", "list", "logs", "watch", "bt", "serial-proxy"};
    for (const auto& [command, type] : domains())
        host_cmds.insert(command);

    const std::size_t idx = find_first_positional(args);
    if (idx == args.size())
        return run_global(args);

    const std::string& first = args[idx];
    if (global_cmds.count(first) != 0)
        return run_global(args);

    std::string host;
    std::vector<std::string> rest = args;
    if (host_cmds.count(first) != 0) {
        const char* env = std::getenv("ESPHOME_CLI_HOST");
        if (env == nullptr || *env == '\0') {
            std::cerr << "error: no host specified. Provide <host> before '" << first
                      << "', or set ESPHOME_CLI_HOST.\n";
            return 2;
        }
        host = env;
    } else {
        host = first;
        rest.erase(rest.begin() + static_cast<std::ptrdiff_t>(idx));
    }
    return run_host(host, rest);
}

}  // namespace cli
