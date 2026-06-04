#include "commands/domains.hpp"

namespace cli {

using esphome::api::EntityType;

const std::vector<DomainEntry>& domains() {
    static const std::vector<DomainEntry> table = {
        {"binary_sensor", EntityType::BinarySensor},
        {"sensor", EntityType::Sensor},
        {"text_sensor", EntityType::TextSensor},
        {"switch", EntityType::Switch},
        {"light", EntityType::Light},
        {"cover", EntityType::Cover},
        {"fan", EntityType::Fan},
        {"climate", EntityType::Climate},
        {"number", EntityType::Number},
        {"select", EntityType::Select},
        {"text", EntityType::Text},
        {"button", EntityType::Button},
        {"lock", EntityType::Lock},
        {"media_player", EntityType::MediaPlayer},
        {"camera", EntityType::Camera},
        {"siren", EntityType::Siren},
        {"alarm_control_panel", EntityType::AlarmControlPanel},
        {"date", EntityType::Date},
        {"time", EntityType::Time},
        {"datetime", EntityType::DateTime},
        {"valve", EntityType::Valve},
        {"event", EntityType::Event},
        {"update", EntityType::Update},
        {"water_heater", EntityType::WaterHeater},
        {"infrared", EntityType::Infrared},
        {"radio_frequency", EntityType::RadioFrequency},
    };
    return table;
}

EntityType domain_to_type(const std::string& command) {
    for (const DomainEntry& e : domains())
        if (command == e.command)
            return e.type;
    return EntityType::Unknown;
}

std::string type_to_domain(const EntityType type) {
    for (const DomainEntry& e : domains())
        if (e.type == type)
            return e.command;
    return {};
}

}  // namespace cli
