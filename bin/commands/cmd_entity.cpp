#include <esphome/api/exception.hpp>
#include <esphome/api/sync_client.hpp>

#include "commands/actions/entity_actions.hpp"
#include "commands/commands.hpp"
#include "commands/domains.hpp"
#include "io/json_model.hpp"
#include "io/yaml_emit.hpp"
#include "net/connection.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <ostream>

namespace cli {
namespace {

// Universal, read-only pseudo-actions available on every entity.
constexpr auto k_state = "state";
constexpr auto k_info = "info";

void print_valid_actions(const esphome::api::EntityType type, const std::string& domain) {
    std::cerr << "valid actions for " << domain << ":";
    for (const std::string& a : available_actions(type))
        std::cerr << " " << a;
    std::cerr << "\n";
}

const esphome::api::StoredEntity* find_typed(const esphome::api::EntityStore& store,
                                             const esphome::api::EntityType type,
                                             const std::string& oid) {
    for (const auto* e : store.entities())
        if (e->type == type && e->object_id == oid)
            return e;
    return nullptr;
}

std::vector<const esphome::api::StoredEntity*>
domain_entities(const esphome::api::EntityStore& store, const esphome::api::EntityType type) {
    std::vector<const esphome::api::StoredEntity*> out;
    for (const auto* e : store.entities())
        if (e->type == type)
            out.push_back(e);
    // Sorted by a stable key (object_id), so the result is deterministic despite
    // the elements being pointers.
    // NOLINTNEXTLINE(bugprone-nondeterministic-pointer-iteration-order)
    std::sort(out.begin(), out.end(), [](const auto* a, const auto* b) {
        return a->object_id < b->object_id;
    });
    return out;
}

}  // namespace

int cmd_entity(CliContext& ctx,
               const std::string& host,
               const std::string& domain,
               const std::string& object_id,
               const std::string& action,
               const std::vector<std::string>& values) {
    const esphome::api::EntityType type = domain_to_type(domain);

    const bool list_mode = object_id.empty();
    const bool describe_mode = !object_id.empty() && action.empty();
    const bool inspect = action == k_state || action == k_info;

    // Validate an actual action verb before paying for a connection.
    const ActionFn* fn = nullptr;
    if (!list_mode && !describe_mode && !inspect) {
        fn = find_action(type, action);
        if (fn == nullptr) {
            std::cerr << "error: unknown action '" << action << "' for " << domain << "\n";
            print_valid_actions(type, domain);
            return 2;
        }
    }

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

    client.list_entities();
    client.subscribe_states();
    try {
        client.pump_until([] { return false; }, std::chrono::milliseconds(800));
    } catch (const esphome::api::TimeoutError&) {}

    // "<domain>" — list every entity in the domain as "<domain>: <id>: <state>".
    if (list_mode) {
        const auto entities = domain_entities(client.store(), type);
        nlohmann::json body = nlohmann::json::object();
        for (const auto* e : entities)
            body[e->object_id] = entity_detail(client.store(), *e)["state"];
        nlohmann::json view;
        view[domain] = body;
        ctx.out.emit(view, [&](std::ostream& os) {
            if (entities.empty())
                os << "no " << domain << " entities on this device\n";
            else
                os << to_yaml(view) << "\n";
        });
        client.disconnect();
        return 0;
    }

    const esphome::api::StoredEntity* e = find_typed(client.store(), type, object_id);
    if (e == nullptr) {
        std::cerr << "error: no such " << domain << " entity: " << object_id << "\n";
        client.disconnect();
        return 2;
    }

    const std::string label = domain + "." + object_id;

    // "<domain> <id>" — describe the entity (info + state + actions), YAML-style.
    if (describe_mode) {
        nlohmann::json view;
        view[label] = entity_detail(client.store(), *e);
        ctx.out.emit(view, [&](std::ostream& os) { os << to_yaml(view) << "\n"; });
        client.disconnect();
        return 0;
    }

    // "<domain> <id> state|info" — inspect, YAML-style keyed by domain.object_id.
    if (inspect) {
        const char* section = (action == k_state) ? "state" : "info";
        nlohmann::json view;
        view[label] = entity_detail(client.store(), *e)[section];
        ctx.out.emit(view, [&](std::ostream& os) { os << to_yaml(view) << "\n"; });
        client.disconnect();
        return 0;
    }

    // "<domain> <id> <action> [values]" — perform it.
    ActionResult result;
    try {
        result = (*fn)(client, object_id, values);
    } catch (const std::exception& ex) {
        result = {false, ex.what()};
    }
    if (!result.ok) {
        std::cerr << "error: " << result.message << "\n";
        client.disconnect();
        return 2;
    }

    try {
        client.pump_until([] { return false; }, std::chrono::milliseconds(1500));
    } catch (const esphome::api::TimeoutError&) {}

    const esphome::api::StoredEntity* after = find_typed(client.store(), type, object_id);
    nlohmann::json doc = after != nullptr
                             ? entity_to_json(client.store(), *after, true)
                             : nlohmann::json{{"domain", domain}, {"object_id", object_id}};
    doc["action"] = result.message;
    ctx.out.emit(doc, [&](std::ostream& os) {
        os << object_id << "  " << result.message << "  [" << state_summary(doc) << "]\n";
    });

    client.disconnect();
    return 0;
}

}  // namespace cli
