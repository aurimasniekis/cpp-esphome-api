#include <esphome/api/exception.hpp>
#include <esphome/api/sync_client.hpp>

#include "commands/commands.hpp"
#include "commands/domains.hpp"
#include "commands/glob.hpp"
#include "io/json_model.hpp"
#include "net/connection.hpp"
#include "net/loop.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <string>

namespace cli {
namespace {

std::string timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    static_cast<void>(localtime_r(&now, &tm_buf));
#endif
    std::array<char, 16> buf{};
    const std::size_t n = std::strftime(buf.data(), buf.size(), "%H:%M:%S", &tm_buf);
    return std::string(buf.data(), n);
}

}  // namespace

int cmd_watch(CliContext& ctx, const std::string& host, const std::vector<std::string>& patterns) {
    const esphome::api::ClientOptions opt = resolve_options(ctx, host);
    esphome::api::SyncClient client(opt);
    client.set_request_timeout(std::chrono::milliseconds(ctx.request_timeout_ms()));

    try {
        client.connect();
    } catch (const std::exception& e) {
        return report_connect_error(ctx, host, e);
    }
    persist_connection(ctx, host, opt, {});

    client.list_entities();
    try {
        client.pump_until([] { return false; }, std::chrono::milliseconds(800));
    } catch (const esphome::api::TimeoutError&) {}

    const OutputWriter& out = ctx.out;
    client.store().on_state([&](const esphome::api::EntityType type, const std::uint32_t key) {
        const esphome::api::StoredEntity* e = client.store().find(key);
        if (e == nullptr)
            return;
        const std::string label = type_to_domain(type) + "." + e->object_id;
        if (!glob_match_any(patterns, label))
            return;
        nlohmann::json doc = entity_to_json(client.store(), *e, true);
        doc["entity"] = label;
        doc["time"] = timestamp();
        out.stream_line(timestamp() + "  " + label + " = " + state_summary(doc), doc);
    });

    client.subscribe_states();
    run_until_signal(client);
    return 0;
}

}  // namespace cli
