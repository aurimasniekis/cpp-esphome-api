#include "commands/commands.hpp"
#include <nlohmann/json.hpp>

#include <iostream>
#include <ostream>

#include <spdlog/spdlog.h>

namespace cli {
namespace {

nlohmann::json record_json(const std::string& host, const DeviceRecord& rec) {
    return {{"host", host},
            {"name", rec.name},
            {"address", rec.address},
            {"port", rec.port},
            {"has_key", !rec.psk.empty()},
            {"expected_name", rec.expected_name}};
}

int config_path(const CliContext& ctx) {
    ctx.out.emit({{"path", ctx.config.path().string()}},
                 [&](std::ostream& os) { os << ctx.config.path().string() << "\n"; });
    return 0;
}

int config_list(const CliContext& ctx) {
    nlohmann::json doc;
    doc["path"] = ctx.config.path().string();
    doc["defaults"] = {{"output", ctx.config.default_output},
                       {"log_level", ctx.config.default_log_level},
                       {"scan_timeout_ms", ctx.config.scan_timeout_ms},
                       {"timeout_ms", ctx.config.request_timeout_ms}};
    doc["save_keys"] = ctx.config.save_keys;
    nlohmann::json devs = nlohmann::json::array();
    for (const auto& [host, rec] : ctx.config.devices)
        devs.push_back(record_json(host, rec));
    doc["devices"] = devs;

    ctx.out.emit(doc, [&](std::ostream& os) {
        os << "config: " << ctx.config.path().string() << "\n";
        os << "defaults: output=" << ctx.config.default_output
           << " log_level=" << ctx.config.default_log_level
           << " scan_timeout_ms=" << ctx.config.scan_timeout_ms
           << " timeout_ms=" << ctx.config.request_timeout_ms << "\n";
        os << "save_keys: " << (ctx.config.save_keys ? "true" : "false") << "\n";
        os << "devices: " << ctx.config.devices.size() << "\n";
        for (const auto& [host, rec] : ctx.config.devices)
            os << "  " << host << " -> " << (rec.address.empty() ? host : rec.address) << ":"
               << rec.port << (rec.psk.empty() ? "" : " [key]") << "\n";
    });
    return 0;
}

int config_get(const CliContext& ctx, const std::string& host) {
    const DeviceRecord* rec = ctx.config.find(host);
    if (rec == nullptr) {
        std::cerr << "error: no saved device matching '" << host << "'\n";
        return 2;
    }
    ctx.out.emit(record_json(host, *rec), [&](std::ostream& os) {
        os << "name:          " << rec->name << "\n";
        os << "address:       " << rec->address << "\n";
        os << "port:          " << rec->port << "\n";
        os << "key:           " << (rec->psk.empty() ? "no" : "yes (stored)") << "\n";
        os << "expected_name: " << rec->expected_name << "\n";
        os << "timeout_ms:    "
           << (rec->timeout_ms ? std::to_string(*rec->timeout_ms) : std::string("(default)"))
           << "\n";
    });
    return 0;
}

int config_set(CliContext& ctx, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "usage: config set <host|defaults> key=value ...\n";
        return 2;
    }
    const std::string& target = args.front();
    bool changed = false;

    if (target == "defaults") {
        for (std::size_t i = 1; i < args.size(); ++i) {
            const auto pos = args[i].find('=');
            if (pos == std::string::npos) {
                std::cerr << "error: expected key=value, got '" << args[i] << "'\n";
                return 2;
            }
            const std::string key = args[i].substr(0, pos);
            const std::string val = args[i].substr(pos + 1);
            if (key == "output") {
                ctx.config.default_output = val;
            } else if (key == "log_level") {
                ctx.config.default_log_level = val;
            } else if (key == "scan_timeout_ms") {
                ctx.config.scan_timeout_ms = std::stoi(val);
            } else if (key == "timeout_ms" || key == "timeout") {
                ctx.config.request_timeout_ms = std::stoi(val);
            } else if (key == "save_keys") {
                ctx.config.save_keys = (val == "true" || val == "1" || val == "yes");
            } else {
                std::cerr << "error: unknown defaults key '" << key << "'\n";
                return 2;
            }
            changed = true;
        }
    } else {
        DeviceRecord rec;
        if (const DeviceRecord* cur = ctx.config.find(target); cur != nullptr)
            rec = *cur;
        if (rec.address.empty())
            rec.address = target;
        for (std::size_t i = 1; i < args.size(); ++i) {
            const auto pos = args[i].find('=');
            if (pos == std::string::npos) {
                std::cerr << "error: expected key=value, got '" << args[i] << "'\n";
                return 2;
            }
            const std::string key = args[i].substr(0, pos);
            const std::string val = args[i].substr(pos + 1);
            if (key == "address") {
                rec.address = val;
            } else if (key == "port") {
                rec.port = static_cast<std::uint16_t>(std::stoi(val));
            } else if (key == "psk" || key == "key") {
                rec.psk = val;
            } else if (key == "name") {
                rec.name = val;
            } else if (key == "expected_name") {
                rec.expected_name = val;
            } else if (key == "timeout_ms" || key == "timeout") {
                rec.timeout_ms = std::stoi(val);
            } else {
                std::cerr << "error: unknown device key '" << key << "'\n";
                return 2;
            }
            changed = true;
        }
        // Explicit 'config set' stores keys regardless of save_keys.
        ctx.config.remember(target, rec, true);
    }

    if (changed)
        ctx.config.save();
    return 0;
}

int config_forget(const CliContext& ctx, const std::string& host) {
    if (!ctx.config.forget(host)) {
        std::cerr << "error: no saved device matching '" << host << "'\n";
        return 2;
    }
    ctx.config.save();
    ctx.log->info("forgot device {}", host);
    return 0;
}

}  // namespace

int cmd_config(CliContext& ctx, const std::string& action, const std::vector<std::string>& args) {
    if (action.empty() || action == "list")
        return config_list(ctx);
    if (action == "path")
        return config_path(ctx);
    if (action == "get") {
        if (args.empty()) {
            std::cerr << "usage: config get <host>\n";
            return 2;
        }
        return config_get(ctx, args.front());
    }
    if (action == "set")
        return config_set(ctx, args);
    if (action == "forget") {
        if (args.empty()) {
            std::cerr << "usage: config forget <host>\n";
            return 2;
        }
        return config_forget(ctx, args.front());
    }
    std::cerr << "error: unknown config action '" << action
              << "' (expected list|get|set|forget|path)\n";
    return 2;
}

}  // namespace cli
