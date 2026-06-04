#include <esphome/api/exception.hpp>
#include <esphome/api/json.hpp>
#include <esphome/api/sync_client.hpp>

#include "commands/commands.hpp"
#include "net/connection.hpp"
#include "net/loop.hpp"

#include <chrono>
#include <string>

namespace cli {
namespace {

esphome::api::LogLevel parse_log_level(const std::string& name) {
    using L = esphome::api::LogLevel;
    if (name == "none")
        return L::None;
    if (name == "error" || name == "err")
        return L::Error;
    if (name == "warn" || name == "warning")
        return L::Warn;
    if (name == "info" || name.empty())
        return L::Info;
    if (name == "config")
        return L::Config;
    if (name == "debug")
        return L::Debug;
    if (name == "verbose")
        return L::Verbose;
    if (name == "very_verbose" || name == "veryverbose")
        return L::VeryVerbose;
    return L::Info;
}

}  // namespace

int cmd_logs(CliContext& ctx, const std::string& host, const std::string& level) {
    const esphome::api::ClientOptions opt = resolve_options(ctx, host);
    esphome::api::SyncClient client(opt);
    client.set_request_timeout(std::chrono::milliseconds(ctx.request_timeout_ms()));

    try {
        client.connect();
    } catch (const std::exception& e) {
        return report_connect_error(ctx, host, e);
    }
    persist_connection(ctx, host, opt, {});

    const OutputWriter& out = ctx.out;
    client.logs().subscribe(parse_log_level(level), [&out](const esphome::api::LogEntry& entry) {
        out.stream_line(entry.message, entry);
    });

    run_until_signal(client);
    return 0;
}

}  // namespace cli
