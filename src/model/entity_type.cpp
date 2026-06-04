#include <esphome/api/model/entity_type.hpp>

#include <unordered_map>

namespace esphome::api {

const char* entity_type_name(const EntityType type) {
    switch (type) {
    case EntityType::BinarySensor:
        return "BinarySensor";
    case EntityType::Sensor:
        return "Sensor";
    case EntityType::TextSensor:
        return "TextSensor";
    case EntityType::Switch:
        return "Switch";
    case EntityType::Light:
        return "Light";
    case EntityType::Cover:
        return "Cover";
    case EntityType::Fan:
        return "Fan";
    case EntityType::Climate:
        return "Climate";
    case EntityType::Number:
        return "Number";
    case EntityType::Select:
        return "Select";
    case EntityType::Text:
        return "Text";
    case EntityType::Button:
        return "Button";
    case EntityType::Lock:
        return "Lock";
    case EntityType::MediaPlayer:
        return "MediaPlayer";
    case EntityType::Camera:
        return "Camera";
    case EntityType::Siren:
        return "Siren";
    case EntityType::AlarmControlPanel:
        return "AlarmControlPanel";
    case EntityType::Date:
        return "Date";
    case EntityType::Time:
        return "Time";
    case EntityType::DateTime:
        return "DateTime";
    case EntityType::Valve:
        return "Valve";
    case EntityType::Event:
        return "Event";
    case EntityType::Update:
        return "Update";
    case EntityType::WaterHeater:
        return "WaterHeater";
    case EntityType::Infrared:
        return "Infrared";
    case EntityType::RadioFrequency:
        return "RadioFrequency";
    case EntityType::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

EntityType entity_type_from_token(const std::string& token) {
    static const std::unordered_map<std::string, EntityType> map = {
        {"BinarySensor", EntityType::BinarySensor},
        {"Sensor", EntityType::Sensor},
        {"TextSensor", EntityType::TextSensor},
        {"Switch", EntityType::Switch},
        {"Light", EntityType::Light},
        {"Cover", EntityType::Cover},
        {"Fan", EntityType::Fan},
        {"Climate", EntityType::Climate},
        {"Number", EntityType::Number},
        {"Select", EntityType::Select},
        {"Text", EntityType::Text},
        {"Button", EntityType::Button},
        {"Lock", EntityType::Lock},
        {"MediaPlayer", EntityType::MediaPlayer},
        {"Camera", EntityType::Camera},
        {"Siren", EntityType::Siren},
        {"AlarmControlPanel", EntityType::AlarmControlPanel},
        {"Date", EntityType::Date},
        {"Time", EntityType::Time},
        {"DateTime", EntityType::DateTime},
        {"Valve", EntityType::Valve},
        {"Event", EntityType::Event},
        {"Update", EntityType::Update},
        {"WaterHeater", EntityType::WaterHeater},
        {"Infrared", EntityType::Infrared},
        {"RadioFrequency", EntityType::RadioFrequency},
    };
    const auto it = map.find(token);
    return it == map.end() ? EntityType::Unknown : it->second;
}

}  // namespace esphome::api
