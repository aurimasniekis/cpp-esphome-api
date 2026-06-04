#pragma once

/// @file
/// @brief Typed store of a device's entities: ingest ListEntities*/State messages,
///        query typed info/state per entity. Generated section — see scripts.

#include <esphome/api/model/entities/alarm_control_panel.hpp>
#include <esphome/api/model/entities/binary_sensor.hpp>
#include <esphome/api/model/entities/button.hpp>
#include <esphome/api/model/entities/camera.hpp>
#include <esphome/api/model/entities/climate.hpp>
#include <esphome/api/model/entities/cover.hpp>
#include <esphome/api/model/entities/date.hpp>
#include <esphome/api/model/entities/datetime.hpp>
#include <esphome/api/model/entities/event.hpp>
#include <esphome/api/model/entities/fan.hpp>
#include <esphome/api/model/entities/infrared.hpp>
#include <esphome/api/model/entities/light.hpp>
#include <esphome/api/model/entities/lock.hpp>
#include <esphome/api/model/entities/media_player.hpp>
#include <esphome/api/model/entities/number.hpp>
#include <esphome/api/model/entities/radio_frequency.hpp>
#include <esphome/api/model/entities/select.hpp>
#include <esphome/api/model/entities/sensor.hpp>
#include <esphome/api/model/entities/siren.hpp>
#include <esphome/api/model/entities/switch.hpp>
#include <esphome/api/model/entities/text.hpp>
#include <esphome/api/model/entities/text_sensor.hpp>
#include <esphome/api/model/entities/time.hpp>
#include <esphome/api/model/entities/update.hpp>
#include <esphome/api/model/entities/valve.hpp>
#include <esphome/api/model/entities/water_heater.hpp>
#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/entity_type.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace esphome::api {

/// A single tracked entity: its domain, identity, and the latest raw info/
/// state protobuf messages (decoded on demand by the typed accessors).
struct StoredEntity {
    EntityType type = EntityType::Unknown;
    std::uint32_t key = 0;
    std::string object_id;
    std::string name;
    std::shared_ptr<ProtoMessage> info;
    std::shared_ptr<ProtoMessage> state;
};

/// Collects a device's entity descriptions and live states. Feed every
/// inbound message to ingest(); query with the typed accessors below.
class EntityStore {
public:
    using StateCallback = std::function<void(EntityType type, std::uint32_t key)>;

    /// Ingest one decoded message. ListEntities*Response populates entity info;
    /// *StateResponse (and EventResponse) updates state and fires on_state.
    /// Returns true if the message was an entity info/state message.
    bool ingest(const ProtoMessage& msg);

    /// Remove all tracked entities.
    void clear();

    /// Fired after each state update.
    void on_state(StateCallback cb) {
        state_cb_ = std::move(cb);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return entities_.size();
    }
    [[nodiscard]] const StoredEntity* find(std::uint32_t key) const;
    [[nodiscard]] const StoredEntity* find_by_object_id(const std::string& object_id) const;
    /// All tracked entities (unordered).
    [[nodiscard]] std::vector<const StoredEntity*> entities() const;

    [[nodiscard]] std::optional<BinarySensorInfo> binary_sensor_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<BinarySensorState> binary_sensor_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<SensorInfo> sensor_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<SensorState> sensor_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<TextSensorInfo> text_sensor_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<TextSensorState> text_sensor_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<SwitchInfo> switch_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<SwitchState> switch_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<LightInfo> light_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<LightState> light_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<CoverInfo> cover_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<CoverState> cover_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<FanInfo> fan_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<FanState> fan_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<ClimateInfo> climate_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<ClimateState> climate_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<NumberInfo> number_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<NumberState> number_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<SelectInfo> select_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<SelectState> select_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<TextInfo> text_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<TextState> text_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<ButtonInfo> button_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<LockInfo> lock_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<LockEntityState> lock_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<MediaPlayerInfo> media_player_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<MediaPlayerStatus> media_player_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<CameraInfo> camera_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<SirenInfo> siren_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<SirenState> siren_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<AlarmControlPanelInfo>
    alarm_control_panel_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<AlarmControlPanelStatus>
    alarm_control_panel_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<DateInfo> date_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<DateState> date_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<TimeInfo> time_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<TimeState> time_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<DateTimeInfo> datetime_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<DateTimeState> datetime_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<ValveInfo> valve_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<ValveState> valve_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<EventInfo> event_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<EventState> event_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<UpdateInfo> update_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<UpdateState> update_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<WaterHeaterInfo> water_heater_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<WaterHeaterState> water_heater_state(std::uint32_t key) const;
    [[nodiscard]] std::optional<InfraredInfo> infrared_info(std::uint32_t key) const;
    [[nodiscard]] std::optional<RadioFrequencyInfo> radio_frequency_info(std::uint32_t key) const;

private:
    std::unordered_map<std::uint32_t, StoredEntity> entities_;
    StateCallback state_cb_;
};

}  // namespace esphome::api
