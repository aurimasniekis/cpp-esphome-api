#include "commands/actions/entity_actions.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace cli {

using esphome::api::EntityType;
using esphome::api::LightCommand;
using esphome::api::LightRgb;
using esphome::api::SyncClient;

namespace {

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

[[noreturn]] void bad_arg(const std::string& what) {
    throw std::invalid_argument(what);
}

ActionResult ok(std::string message) {
    return {true, std::move(message)};
}

ActionResult fail(std::string message) {
    return {false, std::move(message)};
}

}  // namespace

float parse_float(const std::string& text) {
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0')
        bad_arg("not a number: " + text);
    return value;
}

std::int32_t parse_int(const std::string& text) {
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0')
        bad_arg("not an integer: " + text);
    return static_cast<std::int32_t>(value);
}

std::uint32_t parse_uint(const std::string& text) {
    const std::int32_t value = parse_int(text);
    if (value < 0)
        bad_arg("not a non-negative integer: " + text);
    return static_cast<std::uint32_t>(value);
}

float parse_percent_or_unit(const std::string& text) {
    float value = 0.0F;
    if (!text.empty() && text.back() == '%')
        value = parse_float(text.substr(0, text.size() - 1)) / 100.0F;
    else
        value = parse_float(text);
    return std::clamp(value, 0.0F, 1.0F);
}

bool parse_bool(const std::string& text) {
    const std::string s = to_lower(text);
    if (s == "true" || s == "on" || s == "yes" || s == "1")
        return true;
    if (s == "false" || s == "off" || s == "no" || s == "0")
        return false;
    bad_arg("not a boolean: " + text);
}

LightRgb parse_rgb(const std::vector<std::string>& values) {
    std::vector<std::string> parts;
    if (values.size() == 1) {
        std::stringstream ss(values[0]);
        std::string token;
        while (std::getline(ss, token, ','))
            parts.push_back(token);
    } else {
        parts = values;
    }
    if (parts.size() != 3)
        bad_arg("rgb expects 'r,g,b' or three values");

    std::array<float, 3> comp{};
    for (std::size_t i = 0; i < 3; ++i) {
        float v = parse_float(parts[i]);
        if (v > 1.0F)
            v /= 255.0F;
        comp[i] = std::clamp(v, 0.0F, 1.0F);
    }
    return LightRgb{comp[0], comp[1], comp[2]};
}

std::uint64_t parse_ble_addr(const std::string& text) {
    if (text.find(':') != std::string::npos) {
        std::uint64_t addr = 0;
        std::stringstream ss(text);
        std::string byte;
        int count = 0;
        while (std::getline(ss, byte, ':')) {
            addr = (addr << 8) | (std::strtoul(byte.c_str(), nullptr, 16) & 0xFFU);
            ++count;
        }
        if (count != 6)
            bad_arg("expected a 6-byte MAC like AA:BB:CC:DD:EE:FF");
        return addr;
    }
    const int base = (text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0) ? 16 : 10;
    char* end = nullptr;
    const std::uint64_t addr = std::strtoull(text.c_str(), &end, base);
    if (end == text.c_str() || *end != '\0')
        bad_arg("not a BLE address: " + text);
    return addr;
}

namespace {

// --- per-domain command-token helpers --------------------------------------

esphome::api::ClimateMode parse_climate_mode(const std::string& text) {
    using M = esphome::api::ClimateMode;
    const std::string s = to_lower(text);
    if (s == "off")
        return M::Off;
    if (s == "heat_cool" || s == "heatcool" || s == "auto_heat_cool")
        return M::HeatCool;
    if (s == "cool")
        return M::Cool;
    if (s == "heat")
        return M::Heat;
    if (s == "fan_only" || s == "fan")
        return M::FanOnly;
    if (s == "dry")
        return M::Dry;
    if (s == "auto")
        return M::Auto;
    bad_arg("unknown climate mode: " + text);
}

std::pair<std::string, std::string> split_kv(const std::string& token) {
    const auto pos = token.find('=');
    if (pos == std::string::npos)
        bad_arg("expected key=value, got: " + token);
    return {token.substr(0, pos), token.substr(pos + 1)};
}

ActionResult light_command(SyncClient& client,
                           const std::string& object_id,
                           const std::vector<std::string>& values) {
    const auto h = client.entities().light(object_id);
    if (!h)
        return fail("no such light entity: " + object_id);
    LightCommand cmd;
    for (const std::string& token : values) {
        if (const auto [key, value] = split_kv(token); key == "state")
            cmd.state = parse_bool(value);
        else if (key == "brightness")
            cmd.brightness = parse_percent_or_unit(value);
        else if (key == "color_brightness")
            cmd.color_brightness = parse_percent_or_unit(value);
        else if (key == "rgb")
            cmd.rgb = parse_rgb({value});
        else if (key == "white")
            cmd.white = parse_percent_or_unit(value);
        else if (key == "color_temperature")
            cmd.color_temperature = parse_float(value);
        else if (key == "cold_white")
            cmd.cold_white = parse_percent_or_unit(value);
        else if (key == "warm_white")
            cmd.warm_white = parse_percent_or_unit(value);
        else if (key == "transition" || key == "transition_length")
            cmd.transition_length = parse_uint(value);
        else if (key == "flash" || key == "flash_length")
            cmd.flash_length = parse_uint(value);
        else if (key == "effect")
            cmd.effect = value;
        else
            return fail("unknown light command field: " + key);
    }
    h->command(cmd);
    return ok("command");
}

ActionResult climate_command(SyncClient& client,
                             const std::string& object_id,
                             const std::vector<std::string>& values) {
    const auto h = client.entities().climate(object_id);
    if (!h)
        return fail("no such climate entity: " + object_id);
    esphome::api::ClimateCommand cmd;
    for (const std::string& token : values) {
        if (const auto [key, value] = split_kv(token); key == "mode")
            cmd.mode = parse_climate_mode(value);
        else if (key == "target_temperature" || key == "temperature")
            cmd.target_temperature = parse_float(value);
        else if (key == "target_temperature_low")
            cmd.target_temperature_low = parse_float(value);
        else if (key == "target_temperature_high")
            cmd.target_temperature_high = parse_float(value);
        else if (key == "target_humidity" || key == "humidity")
            cmd.target_humidity = parse_float(value);
        else
            return fail("unknown climate command field: " + key);
    }
    h->command(cmd);
    return ok("command");
}

// Resolve a handle (binding `h`) or early-return a failure. `id` is the
// object-id parameter every action lambda receives.
#define CLI_NEED(handle_expr, label)                                                               \
    auto h = (handle_expr);                                                                        \
    if (!h)                                                                                        \
    return fail(std::string("no such ") + (label) + " entity: " + id)

using Table = std::map<EntityType, std::map<std::string, ActionFn>>;

Table build_table() {
    Table t;

    t[EntityType::Switch] = {
        {"turn_on",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().switch_(id), "switch");
             h->turn_on();
             return ok("turn_on");
         }},
        {"turn_off",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().switch_(id), "switch");
             h->turn_off();
             return ok("turn_off");
         }},
        {"toggle",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().switch_(id), "switch");
             h->toggle();
             return ok("toggle");
         }},
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().switch_(id), "switch");
             if (v.empty())
                 return fail("set requires a boolean value");
             h->set_power(parse_bool(v[0]));
             return ok("set");
         }},
    };

    t[EntityType::Light] = {
        {"turn_on",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().light(id), "light");
             if (!v.empty())
                 h->set_brightness(parse_percent_or_unit(v[0]));
             else
                 h->turn_on();
             return ok("turn_on");
         }},
        {"turn_off",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().light(id), "light");
             h->turn_off();
             return ok("turn_off");
         }},
        {"toggle",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().light(id), "light");
             h->toggle();
             return ok("toggle");
         }},
        {"set_brightness",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().light(id), "light");
             if (v.empty())
                 return fail("set_brightness requires a level (e.g. 50% or 0.5)");
             h->set_brightness(parse_percent_or_unit(v[0]));
             return ok("set_brightness");
         }},
        {"set_rgb",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().light(id), "light");
             const auto [red, green, blue] = parse_rgb(v);
             h->set_rgb(red, green, blue);
             return ok("set_rgb");
         }},
        {"command", light_command},
    };

    t[EntityType::Fan] = {
        {"turn_on",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().fan(id), "fan");
             h->turn_on();
             return ok("turn_on");
         }},
        {"turn_off",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().fan(id), "fan");
             h->turn_off();
             return ok("turn_off");
         }},
        {"toggle",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().fan(id), "fan");
             h->toggle();
             return ok("toggle");
         }},
        {"set_speed",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().fan(id), "fan");
             if (v.empty())
                 return fail("set_speed requires a level");
             h->set_speed(parse_int(v[0]));
             return ok("set_speed");
         }},
    };

    t[EntityType::Cover] = {
        {"open",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().cover(id), "cover");
             h->open();
             return ok("open");
         }},
        {"close",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().cover(id), "cover");
             h->close();
             return ok("close");
         }},
        {"stop",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().cover(id), "cover");
             h->stop();
             return ok("stop");
         }},
        {"set_position",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().cover(id), "cover");
             if (v.empty())
                 return fail("set_position requires a position (e.g. 50% or 0.5)");
             h->set_position(parse_percent_or_unit(v[0]));
             return ok("set_position");
         }},
    };

    t[EntityType::Valve] = {
        {"open",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().valve(id), "valve");
             h->open();
             return ok("open");
         }},
        {"close",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().valve(id), "valve");
             h->close();
             return ok("close");
         }},
        {"stop",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().valve(id), "valve");
             h->stop();
             return ok("stop");
         }},
        {"set_position",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().valve(id), "valve");
             if (v.empty())
                 return fail("set_position requires a position");
             esphome::api::ValveCommand cmd;
             cmd.position = parse_percent_or_unit(v[0]);
             h->command(cmd);
             return ok("set_position");
         }},
    };

    t[EntityType::Climate] = {
        {"set_mode",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().climate(id), "climate");
             if (v.empty())
                 return fail("set_mode requires a mode");
             h->set_mode(parse_climate_mode(v[0]));
             return ok("set_mode");
         }},
        {"set_target_temperature",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().climate(id), "climate");
             if (v.empty())
                 return fail("set_target_temperature requires a value");
             h->set_target_temperature(parse_float(v[0]));
             return ok("set_target_temperature");
         }},
        {"command", climate_command},
    };

    t[EntityType::Number] = {
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().number(id), "number");
             if (v.empty())
                 return fail("set requires a value");
             h->set(parse_float(v[0]));
             return ok("set");
         }},
    };

    t[EntityType::Select] = {
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().select(id), "select");
             if (v.empty())
                 return fail("set requires an option");
             h->set(v[0]);
             return ok("set");
         }},
    };

    t[EntityType::Text] = {
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().text(id), "text");
             if (v.empty())
                 return fail("set requires a value");
             h->set(v[0]);
             return ok("set");
         }},
    };

    t[EntityType::Button] = {
        {"press",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().button(id), "button");
             h->press();
             return ok("press");
         }},
    };

    t[EntityType::Lock] = {
        {"lock",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().lock(id), "lock");
             h->lock();
             return ok("lock");
         }},
        {"unlock",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().lock(id), "lock");
             h->unlock();
             return ok("unlock");
         }},
        {"open",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().lock(id), "lock");
             h->open();
             return ok("open");
         }},
    };

    t[EntityType::MediaPlayer] = {
        {"play",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().media_player(id), "media_player");
             h->play();
             return ok("play");
         }},
        {"pause",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().media_player(id), "media_player");
             h->pause();
             return ok("pause");
         }},
        {"stop",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().media_player(id), "media_player");
             h->stop();
             return ok("stop");
         }},
        {"set_volume",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().media_player(id), "media_player");
             if (v.empty())
                 return fail("set_volume requires a level (e.g. 50% or 0.5)");
             h->set_volume(parse_percent_or_unit(v[0]));
             return ok("set_volume");
         }},
    };

    t[EntityType::Siren] = {
        {"turn_on",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().siren(id), "siren");
             h->turn_on();
             return ok("turn_on");
         }},
        {"turn_off",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().siren(id), "siren");
             h->turn_off();
             return ok("turn_off");
         }},
        {"toggle",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().siren(id), "siren");
             h->toggle();
             return ok("toggle");
         }},
    };

    t[EntityType::AlarmControlPanel] = {
        {"arm_away",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().alarm_control_panel(id), "alarm_control_panel");
             h->arm_away(v.empty() ? std::string{} : v[0]);
             return ok("arm_away");
         }},
        {"arm_home",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().alarm_control_panel(id), "alarm_control_panel");
             h->arm_home(v.empty() ? std::string{} : v[0]);
             return ok("arm_home");
         }},
        {"disarm",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().alarm_control_panel(id), "alarm_control_panel");
             h->disarm(v.empty() ? std::string{} : v[0]);
             return ok("disarm");
         }},
    };

    t[EntityType::Date] = {
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().date(id), "date");
             if (v.empty())
                 return fail("set requires a date (YYYY-MM-DD)");
             std::vector<std::string> p;
             std::stringstream ss(v[0]);
             std::string tok;
             while (std::getline(ss, tok, '-'))
                 p.push_back(tok);
             if (p.size() != 3)
                 return fail("date must be YYYY-MM-DD");
             h->set(parse_uint(p[0]), parse_uint(p[1]), parse_uint(p[2]));
             return ok("set");
         }},
    };

    t[EntityType::Time] = {
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().time(id), "time");
             if (v.empty())
                 return fail("set requires a time (HH:MM[:SS])");
             std::vector<std::string> p;
             std::stringstream ss(v[0]);
             std::string tok;
             while (std::getline(ss, tok, ':'))
                 p.push_back(tok);
             if (p.size() < 2)
                 return fail("time must be HH:MM[:SS]");
             h->set(parse_uint(p[0]), parse_uint(p[1]), p.size() > 2 ? parse_uint(p[2]) : 0U);
             return ok("set");
         }},
    };

    t[EntityType::DateTime] = {
        {"set",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>& v) {
             CLI_NEED(c.entities().datetime(id), "datetime");
             if (v.empty())
                 return fail("set requires an epoch-seconds value");
             h->set(parse_uint(v[0]));
             return ok("set");
         }},
    };

    t[EntityType::Update] = {
        {"install",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().update(id), "update");
             h->install();
             return ok("install");
         }},
        {"check",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().update(id), "update");
             h->check();
             return ok("check");
         }},
    };

    t[EntityType::Camera] = {
        {"request_image",
         [](SyncClient& c, const std::string& id, const std::vector<std::string>&) {
             CLI_NEED(c.entities().camera(id), "camera");
             h->request_image();
             return ok("request_image");
         }},
    };

    return t;
}

const Table& table() {
    static const Table t = build_table();
    return t;
}

}  // namespace

const ActionFn* find_action(const EntityType type, const std::string& action) {
    const auto domain = table().find(type);
    if (domain == table().end())
        return nullptr;
    const auto fn = domain->second.find(action);
    return fn == domain->second.end() ? nullptr : &fn->second;
}

std::vector<std::string> actions_for(const EntityType type) {
    std::vector<std::string> names;
    if (const auto domain = table().find(type); domain != table().end())
        for (const auto& [name, fn] : domain->second)
            names.push_back(name);
    return names;
}

std::vector<std::string> available_actions(const EntityType type) {
    std::vector<std::string> names = actions_for(type);
    names.emplace_back("info");
    names.emplace_back("state");
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace cli
