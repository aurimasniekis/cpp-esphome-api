#include <esphome/api/exception.hpp>
#include <esphome/api/json.hpp>
#include <esphome/api/sync_client.hpp>

#include "commands/commands.hpp"
#include "commands/domains.hpp"
#include "io/json_model.hpp"
#include "io/yaml_emit.hpp"
#include "net/connection.hpp"

#include <algorithm>
#include <chrono>
#include <ostream>

namespace cli {

int cmd_info(CliContext& ctx, const std::string& host, const bool entities) {
    const esphome::api::ClientOptions opt = resolve_options(ctx, host);
    esphome::api::SyncClient client(opt);
    client.set_request_timeout(std::chrono::milliseconds(ctx.request_timeout_ms()));

    try {
        client.connect();
    } catch (const std::exception& e) {
        return report_connect_error(ctx, host, e);
    }

    const esphome::api::DeviceInfo info = client.device_info();
    persist_connection(ctx, host, opt, info.name);

    const std::string device_key = info.name.empty() ? host : info.name;

    // Top-level document: the device keyed by its name, plus an optional
    // separate "entities" map (rendered as its own YAML block in text mode).
    nlohmann::json doc;
    doc[device_key] = info;

    if (entities) {
        client.list_entities();
        client.subscribe_states();
        try {
            client.pump_until([] { return false; }, std::chrono::milliseconds(1200));
        } catch (const esphome::api::TimeoutError&) {}

        auto stored = client.store().entities();
        // Sorted by a stable key (type, object_id) → deterministic order.
        // NOLINTNEXTLINE(bugprone-nondeterministic-pointer-iteration-order)
        std::sort(stored.begin(), stored.end(), [](const auto* a, const auto* b) {
            if (a->type != b->type)
                return static_cast<int>(a->type) < static_cast<int>(b->type);
            return a->object_id < b->object_id;
        });

        nlohmann::json ents = nlohmann::json::object();
        for (const auto* e : stored)
            ents[type_to_domain(e->type) + "." + e->object_id] = entity_detail(client.store(), *e);
        doc["entities"] = ents;
    }

    ctx.out.emit(doc, [&](std::ostream& os) {
        nlohmann::json device;
        device[device_key] = doc[device_key];
        os << to_yaml(device) << "\n";
        if (doc.contains("entities")) {
            nlohmann::json wrap;
            wrap["entities"] = doc["entities"];
            os << "\n" << to_yaml(wrap) << "\n";
        }
    });

    client.disconnect();
    return 0;
}

}  // namespace cli
