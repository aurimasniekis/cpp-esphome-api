#include "io/json_model.hpp"

#include <esphome/api/json.hpp>

#include "commands/actions/entity_actions.hpp"
#include "commands/domains.hpp"

#include <sstream>

namespace cli {

using esphome::api::EntityStore;
using esphome::api::EntityType;
using esphome::api::StoredEntity;

namespace {

// Attach info/state for a domain. INFO/STATE are the EntityStore accessor stems.
#define CLI_INFO(accessor)                                                                         \
    if (auto info = store.accessor(entity.key))                                                    \
    j["info"] = *info
#define CLI_STATE(accessor)                                                                        \
    do {                                                                                           \
        if (auto state = store.accessor(entity.key))                                               \
            j["state"] = *state;                                                                   \
        else                                                                                       \
            j["state"] = nullptr;                                                                  \
    } while (false)

}  // namespace

nlohmann::json
entity_to_json(const EntityStore& store, const StoredEntity& entity, const bool include_state) {
    nlohmann::json j;
    j["domain"] = type_to_domain(entity.type);
    j["object_id"] = entity.object_id;
    j["name"] = entity.name;
    j["key"] = entity.key;

    switch (entity.type) {
    case EntityType::BinarySensor:
        CLI_INFO(binary_sensor_info);
        if (include_state)
            CLI_STATE(binary_sensor_state);
        break;
    case EntityType::Sensor:
        CLI_INFO(sensor_info);
        if (include_state)
            CLI_STATE(sensor_state);
        break;
    case EntityType::TextSensor:
        CLI_INFO(text_sensor_info);
        if (include_state)
            CLI_STATE(text_sensor_state);
        break;
    case EntityType::Switch:
        CLI_INFO(switch_info);
        if (include_state)
            CLI_STATE(switch_state);
        break;
    case EntityType::Light:
        CLI_INFO(light_info);
        if (include_state)
            CLI_STATE(light_state);
        break;
    case EntityType::Cover:
        CLI_INFO(cover_info);
        if (include_state)
            CLI_STATE(cover_state);
        break;
    case EntityType::Fan:
        CLI_INFO(fan_info);
        if (include_state)
            CLI_STATE(fan_state);
        break;
    case EntityType::Climate:
        CLI_INFO(climate_info);
        if (include_state)
            CLI_STATE(climate_state);
        break;
    case EntityType::Number:
        CLI_INFO(number_info);
        if (include_state)
            CLI_STATE(number_state);
        break;
    case EntityType::Select:
        CLI_INFO(select_info);
        if (include_state)
            CLI_STATE(select_state);
        break;
    case EntityType::Text:
        CLI_INFO(text_info);
        if (include_state)
            CLI_STATE(text_state);
        break;
    case EntityType::Button:
        CLI_INFO(button_info);
        if (include_state)
            j["state"] = nullptr;
        break;
    case EntityType::Lock:
        CLI_INFO(lock_info);
        if (include_state)
            CLI_STATE(lock_state);
        break;
    case EntityType::MediaPlayer:
        CLI_INFO(media_player_info);
        if (include_state)
            CLI_STATE(media_player_state);
        break;
    case EntityType::Camera:
        CLI_INFO(camera_info);
        if (include_state)
            j["state"] = nullptr;
        break;
    case EntityType::Siren:
        CLI_INFO(siren_info);
        if (include_state)
            CLI_STATE(siren_state);
        break;
    case EntityType::AlarmControlPanel:
        CLI_INFO(alarm_control_panel_info);
        if (include_state)
            CLI_STATE(alarm_control_panel_state);
        break;
    case EntityType::Date:
        CLI_INFO(date_info);
        if (include_state)
            CLI_STATE(date_state);
        break;
    case EntityType::Time:
        CLI_INFO(time_info);
        if (include_state)
            CLI_STATE(time_state);
        break;
    case EntityType::DateTime:
        CLI_INFO(datetime_info);
        if (include_state)
            CLI_STATE(datetime_state);
        break;
    case EntityType::Valve:
        CLI_INFO(valve_info);
        if (include_state)
            CLI_STATE(valve_state);
        break;
    case EntityType::Event:
        CLI_INFO(event_info);
        if (include_state)
            CLI_STATE(event_state);
        break;
    case EntityType::Update:
        CLI_INFO(update_info);
        if (include_state)
            CLI_STATE(update_state);
        break;
    case EntityType::WaterHeater:
        CLI_INFO(water_heater_info);
        if (include_state)
            CLI_STATE(water_heater_state);
        break;
    case EntityType::Infrared:
        CLI_INFO(infrared_info);
        if (include_state)
            j["state"] = nullptr;
        break;
    case EntityType::RadioFrequency:
        CLI_INFO(radio_frequency_info);
        if (include_state)
            j["state"] = nullptr;
        break;
    case EntityType::Unknown:
        break;
    }
    return j;
}

#undef CLI_INFO
#undef CLI_STATE

namespace {

std::string scalar_text(const nlohmann::json& v) {
    if (v.is_string())
        return v.get<std::string>();
    if (v.is_boolean())
        return v.get<bool>() ? "true" : "false";
    return v.dump();
}

}  // namespace

namespace {

nlohmann::json without_key(nlohmann::json obj) {
    if (obj.is_object())
        obj.erase("key");
    return obj;
}

}  // namespace

nlohmann::json entity_detail(const EntityStore& store, const StoredEntity& entity) {
    const nlohmann::json full = entity_to_json(store, entity, true);
    nlohmann::json body;
    body["info"] = without_key(full.value("info", nlohmann::json::object()));
    body["state"] = full.contains("state") ? without_key(full["state"]) : nlohmann::json(nullptr);
    body["actions"] = available_actions(entity.type);
    return body;
}

std::string state_summary(const nlohmann::json& entity_doc) {
    const auto it = entity_doc.find("state");
    if (it == entity_doc.end() || it->is_null() || !it->is_object())
        return "-";

    std::ostringstream os;
    bool first = true;
    for (auto field = it->begin(); field != it->end(); ++field) {
        if (field.key() == "key")
            continue;
        if (!first)
            os << " ";
        first = false;
        os << field.key() << "=" << scalar_text(field.value());
    }
    const std::string out = os.str();
    return out.empty() ? "-" : out;
}

}  // namespace cli
