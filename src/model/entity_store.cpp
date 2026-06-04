#include <esphome/api/model/entity_store.hpp>
#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace esphome::api {

namespace {
std::uint32_t reflect_u32(const ProtoMessage& msg, const char* field) {
    const auto* fd = msg.GetDescriptor()->FindFieldByName(field);
    if (fd == nullptr)
        return 0;
    return msg.GetReflection()->GetUInt32(msg, fd);
}
std::string reflect_str(const ProtoMessage& msg, const char* field) {
    const auto* fd = msg.GetDescriptor()->FindFieldByName(field);
    if (fd == nullptr)
        return {};
    return msg.GetReflection()->GetString(msg, fd);
}
std::shared_ptr<ProtoMessage> clone(const ProtoMessage& msg) {
    std::shared_ptr<ProtoMessage> copy(msg.New());
    copy->CopyFrom(msg);
    return copy;
}
}  // namespace

bool EntityStore::ingest(const ProtoMessage& msg) {
    const std::string name(msg.GetDescriptor()->name());
    const bool is_list = name.rfind("ListEntities", 0) == 0 && name.size() > 8 &&
                         name.substr(name.size() - 8) == "Response" &&
                         name != "ListEntitiesDoneResponse" &&
                         name != "ListEntitiesServicesResponse";
    const bool is_event = name == "EventResponse";
    const bool is_state =
        is_event || (name.size() > 13 && name.substr(name.size() - 13) == "StateResponse");
    if (is_list) {
        const std::string token = name.substr(12, name.size() - 12 - 8);
        const std::uint32_t key = reflect_u32(msg, "key");
        StoredEntity& e = entities_[key];
        e.type = entity_type_from_token(token);
        e.key = key;
        e.name = reflect_str(msg, "name");
        e.object_id = reflect_str(msg, "object_id");
        if (e.object_id.empty())
            e.object_id = object_id_from_name(e.name);
        e.info = clone(msg);
        return true;
    }
    if (is_state) {
        const std::string token = is_event ? "Event" : name.substr(0, name.size() - 13);
        const EntityType type = entity_type_from_token(token);
        const std::uint32_t key = reflect_u32(msg, "key");
        StoredEntity& e = entities_[key];
        if (e.type == EntityType::Unknown)
            e.type = type;
        e.key = key;
        e.state = clone(msg);
        if (state_cb_)
            state_cb_(type, key);
        return true;
    }
    return false;
}

void EntityStore::clear() {
    entities_.clear();
}

const StoredEntity* EntityStore::find(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    return it == entities_.end() ? nullptr : &it->second;
}

const StoredEntity* EntityStore::find_by_object_id(const std::string& object_id) const {
    for (const auto& [k, e] : entities_)
        if (e.object_id == object_id)
            return &e;
    return nullptr;
}

std::vector<const StoredEntity*> EntityStore::entities() const {
    std::vector<const StoredEntity*> out;
    out.reserve(entities_.size());
    for (const auto& [k, e] : entities_)
        out.push_back(&e);
    return out;
}

std::optional<BinarySensorInfo> EntityStore::binary_sensor_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::BinarySensor || !it->second.info)
        return std::nullopt;
    return parse_binary_sensor_info(
        static_cast<const proto::ListEntitiesBinarySensorResponse&>(*it->second.info));
}
std::optional<BinarySensorState> EntityStore::binary_sensor_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::BinarySensor || !it->second.state)
        return std::nullopt;
    return parse_binary_sensor_state(
        static_cast<const proto::BinarySensorStateResponse&>(*it->second.state));
}
std::optional<SensorInfo> EntityStore::sensor_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Sensor || !it->second.info)
        return std::nullopt;
    return parse_sensor_info(
        static_cast<const proto::ListEntitiesSensorResponse&>(*it->second.info));
}
std::optional<SensorState> EntityStore::sensor_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Sensor || !it->second.state)
        return std::nullopt;
    return parse_sensor_state(static_cast<const proto::SensorStateResponse&>(*it->second.state));
}
std::optional<TextSensorInfo> EntityStore::text_sensor_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::TextSensor || !it->second.info)
        return std::nullopt;
    return parse_text_sensor_info(
        static_cast<const proto::ListEntitiesTextSensorResponse&>(*it->second.info));
}
std::optional<TextSensorState> EntityStore::text_sensor_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::TextSensor || !it->second.state)
        return std::nullopt;
    return parse_text_sensor_state(
        static_cast<const proto::TextSensorStateResponse&>(*it->second.state));
}
std::optional<SwitchInfo> EntityStore::switch_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Switch || !it->second.info)
        return std::nullopt;
    return parse_switch_info(
        static_cast<const proto::ListEntitiesSwitchResponse&>(*it->second.info));
}
std::optional<SwitchState> EntityStore::switch_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Switch || !it->second.state)
        return std::nullopt;
    return parse_switch_state(static_cast<const proto::SwitchStateResponse&>(*it->second.state));
}
std::optional<LightInfo> EntityStore::light_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Light || !it->second.info)
        return std::nullopt;
    return parse_light_info(static_cast<const proto::ListEntitiesLightResponse&>(*it->second.info));
}
std::optional<LightState> EntityStore::light_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Light || !it->second.state)
        return std::nullopt;
    return parse_light_state(static_cast<const proto::LightStateResponse&>(*it->second.state));
}
std::optional<CoverInfo> EntityStore::cover_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Cover || !it->second.info)
        return std::nullopt;
    return parse_cover_info(static_cast<const proto::ListEntitiesCoverResponse&>(*it->second.info));
}
std::optional<CoverState> EntityStore::cover_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Cover || !it->second.state)
        return std::nullopt;
    return parse_cover_state(static_cast<const proto::CoverStateResponse&>(*it->second.state));
}
std::optional<FanInfo> EntityStore::fan_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Fan || !it->second.info)
        return std::nullopt;
    return parse_fan_info(static_cast<const proto::ListEntitiesFanResponse&>(*it->second.info));
}
std::optional<FanState> EntityStore::fan_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Fan || !it->second.state)
        return std::nullopt;
    return parse_fan_state(static_cast<const proto::FanStateResponse&>(*it->second.state));
}
std::optional<ClimateInfo> EntityStore::climate_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Climate || !it->second.info)
        return std::nullopt;
    return parse_climate_info(
        static_cast<const proto::ListEntitiesClimateResponse&>(*it->second.info));
}
std::optional<ClimateState> EntityStore::climate_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Climate || !it->second.state)
        return std::nullopt;
    return parse_climate_state(static_cast<const proto::ClimateStateResponse&>(*it->second.state));
}
std::optional<NumberInfo> EntityStore::number_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Number || !it->second.info)
        return std::nullopt;
    return parse_number_info(
        static_cast<const proto::ListEntitiesNumberResponse&>(*it->second.info));
}
std::optional<NumberState> EntityStore::number_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Number || !it->second.state)
        return std::nullopt;
    return parse_number_state(static_cast<const proto::NumberStateResponse&>(*it->second.state));
}
std::optional<SelectInfo> EntityStore::select_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Select || !it->second.info)
        return std::nullopt;
    return parse_select_info(
        static_cast<const proto::ListEntitiesSelectResponse&>(*it->second.info));
}
std::optional<SelectState> EntityStore::select_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Select || !it->second.state)
        return std::nullopt;
    return parse_select_state(static_cast<const proto::SelectStateResponse&>(*it->second.state));
}
std::optional<TextInfo> EntityStore::text_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Text || !it->second.info)
        return std::nullopt;
    return parse_text_info(static_cast<const proto::ListEntitiesTextResponse&>(*it->second.info));
}
std::optional<TextState> EntityStore::text_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Text || !it->second.state)
        return std::nullopt;
    return parse_text_state(static_cast<const proto::TextStateResponse&>(*it->second.state));
}
std::optional<ButtonInfo> EntityStore::button_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Button || !it->second.info)
        return std::nullopt;
    return parse_button_info(
        static_cast<const proto::ListEntitiesButtonResponse&>(*it->second.info));
}
std::optional<LockInfo> EntityStore::lock_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Lock || !it->second.info)
        return std::nullopt;
    return parse_lock_info(static_cast<const proto::ListEntitiesLockResponse&>(*it->second.info));
}
std::optional<LockEntityState> EntityStore::lock_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Lock || !it->second.state)
        return std::nullopt;
    return parse_lock_state(static_cast<const proto::LockStateResponse&>(*it->second.state));
}
std::optional<MediaPlayerInfo> EntityStore::media_player_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::MediaPlayer || !it->second.info)
        return std::nullopt;
    return parse_media_player_info(
        static_cast<const proto::ListEntitiesMediaPlayerResponse&>(*it->second.info));
}
std::optional<MediaPlayerStatus> EntityStore::media_player_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::MediaPlayer || !it->second.state)
        return std::nullopt;
    return parse_media_player_state(
        static_cast<const proto::MediaPlayerStateResponse&>(*it->second.state));
}
std::optional<CameraInfo> EntityStore::camera_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Camera || !it->second.info)
        return std::nullopt;
    return parse_camera_info(
        static_cast<const proto::ListEntitiesCameraResponse&>(*it->second.info));
}
std::optional<SirenInfo> EntityStore::siren_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Siren || !it->second.info)
        return std::nullopt;
    return parse_siren_info(static_cast<const proto::ListEntitiesSirenResponse&>(*it->second.info));
}
std::optional<SirenState> EntityStore::siren_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Siren || !it->second.state)
        return std::nullopt;
    return parse_siren_state(static_cast<const proto::SirenStateResponse&>(*it->second.state));
}
std::optional<AlarmControlPanelInfo>
EntityStore::alarm_control_panel_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::AlarmControlPanel ||
        !it->second.info)
        return std::nullopt;
    return parse_alarm_control_panel_info(
        static_cast<const proto::ListEntitiesAlarmControlPanelResponse&>(*it->second.info));
}
std::optional<AlarmControlPanelStatus>
EntityStore::alarm_control_panel_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::AlarmControlPanel ||
        !it->second.state)
        return std::nullopt;
    return parse_alarm_control_panel_state(
        static_cast<const proto::AlarmControlPanelStateResponse&>(*it->second.state));
}
std::optional<DateInfo> EntityStore::date_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Date || !it->second.info)
        return std::nullopt;
    return parse_date_info(static_cast<const proto::ListEntitiesDateResponse&>(*it->second.info));
}
std::optional<DateState> EntityStore::date_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Date || !it->second.state)
        return std::nullopt;
    return parse_date_state(static_cast<const proto::DateStateResponse&>(*it->second.state));
}
std::optional<TimeInfo> EntityStore::time_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Time || !it->second.info)
        return std::nullopt;
    return parse_time_info(static_cast<const proto::ListEntitiesTimeResponse&>(*it->second.info));
}
std::optional<TimeState> EntityStore::time_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Time || !it->second.state)
        return std::nullopt;
    return parse_time_state(static_cast<const proto::TimeStateResponse&>(*it->second.state));
}
std::optional<DateTimeInfo> EntityStore::datetime_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::DateTime || !it->second.info)
        return std::nullopt;
    return parse_datetime_info(
        static_cast<const proto::ListEntitiesDateTimeResponse&>(*it->second.info));
}
std::optional<DateTimeState> EntityStore::datetime_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::DateTime || !it->second.state)
        return std::nullopt;
    return parse_datetime_state(
        static_cast<const proto::DateTimeStateResponse&>(*it->second.state));
}
std::optional<ValveInfo> EntityStore::valve_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Valve || !it->second.info)
        return std::nullopt;
    return parse_valve_info(static_cast<const proto::ListEntitiesValveResponse&>(*it->second.info));
}
std::optional<ValveState> EntityStore::valve_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Valve || !it->second.state)
        return std::nullopt;
    return parse_valve_state(static_cast<const proto::ValveStateResponse&>(*it->second.state));
}
std::optional<EventInfo> EntityStore::event_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Event || !it->second.info)
        return std::nullopt;
    return parse_event_info(static_cast<const proto::ListEntitiesEventResponse&>(*it->second.info));
}
std::optional<EventState> EntityStore::event_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Event || !it->second.state)
        return std::nullopt;
    return parse_event_state(static_cast<const proto::EventResponse&>(*it->second.state));
}
std::optional<UpdateInfo> EntityStore::update_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Update || !it->second.info)
        return std::nullopt;
    return parse_update_info(
        static_cast<const proto::ListEntitiesUpdateResponse&>(*it->second.info));
}
std::optional<UpdateState> EntityStore::update_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Update || !it->second.state)
        return std::nullopt;
    return parse_update_state(static_cast<const proto::UpdateStateResponse&>(*it->second.state));
}
std::optional<WaterHeaterInfo> EntityStore::water_heater_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::WaterHeater || !it->second.info)
        return std::nullopt;
    return parse_water_heater_info(
        static_cast<const proto::ListEntitiesWaterHeaterResponse&>(*it->second.info));
}
std::optional<WaterHeaterState> EntityStore::water_heater_state(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::WaterHeater || !it->second.state)
        return std::nullopt;
    return parse_water_heater_state(
        static_cast<const proto::WaterHeaterStateResponse&>(*it->second.state));
}
std::optional<InfraredInfo> EntityStore::infrared_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::Infrared || !it->second.info)
        return std::nullopt;
    return parse_infrared_info(
        static_cast<const proto::ListEntitiesInfraredResponse&>(*it->second.info));
}
std::optional<RadioFrequencyInfo> EntityStore::radio_frequency_info(const std::uint32_t key) const {
    const auto it = entities_.find(key);
    if (it == entities_.end() || it->second.type != EntityType::RadioFrequency || !it->second.info)
        return std::nullopt;
    return parse_radio_frequency_info(
        static_cast<const proto::ListEntitiesRadioFrequencyResponse&>(*it->second.info));
}

}  // namespace esphome::api
