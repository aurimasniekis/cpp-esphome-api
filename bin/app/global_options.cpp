#include "app/global_options.hpp"

#include <esphome/api/version.hpp>

#include <CLI/CLI.hpp>

#include <set>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace cli {

const char* cli_version() {
    return ESPHOME_API_VERSION_STRING;
}

std::string client_info() {
    return std::string("esphome-cli/") + cli_version();
}

void add_global_options(CLI::App& app, GlobalOptions& g) {
    app.add_option("--config", g.config_path, "Path to the config file")
        ->envname("ESPHOME_CLI_CONFIG");
    app.add_option("--log-level", g.log_level, "CLI log verbosity")
        ->envname("ESPHOME_CLI_LOG_LEVEL")
        ->check(CLI::IsMember(
            {"trace", "debug", "info", "warn", "warning", "error", "err", "critical", "off"}));
    app.add_option("--output", g.output, "Output format")
        ->envname("ESPHOME_CLI_OUTPUT")
        ->check(CLI::IsMember({"text", "json", "yaml", "yml"}));
    app.add_option("-p,--port", g.port, "API TCP port (default 6053)")->envname("ESPHOME_CLI_PORT");
    app.add_option("-k,--key", g.key, "Base64 Noise PSK (enables encryption)")
        ->envname("ESPHOME_CLI_KEY");
    app.add_option("-n,--name", g.name, "Expected device name (handshake guard)")
        ->envname("ESPHOME_CLI_NAME");
    app.add_flag("--save-keys,!--no-save-keys", g.save_keys, "Persist encryption keys to config")
        ->envname("ESPHOME_CLI_SAVE_KEYS");
    app.add_option("--timeout", g.timeout_ms, "Per-request timeout in milliseconds")
        ->envname("ESPHOME_CLI_TIMEOUT");
}

std::shared_ptr<spdlog::logger> make_logger(const std::string& level) {
    auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("esphome-cli", sink);

    spdlog::level::level_enum lvl = spdlog::level::warn;
    if (level == "trace")
        lvl = spdlog::level::trace;
    else if (level == "debug")
        lvl = spdlog::level::debug;
    else if (level == "info")
        lvl = spdlog::level::info;
    else if (level == "warn" || level == "warning")
        lvl = spdlog::level::warn;
    else if (level == "error" || level == "err")
        lvl = spdlog::level::err;
    else if (level == "critical")
        lvl = spdlog::level::critical;
    else if (level == "off")
        lvl = spdlog::level::off;

    logger->set_level(lvl);
    logger->set_pattern("%^[%l]%$ %v");
    return logger;
}

}  // namespace cli
