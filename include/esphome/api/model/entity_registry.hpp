#pragma once

/// @file
/// @brief EntityRegistry: typed, object-oriented access to the entity store.
///        `client.entities().switches()`, `entities().light("ceiling")`, etc.

#include <esphome/api/client.hpp>
#include <esphome/api/model/entity_handles.hpp>
#include <esphome/api/model/entity_store.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace esphome::api {

/// A snapshot list of entity handles of one domain, with index + lookup access.
template <class E>
class EntityList {
public:
    explicit EntityList(std::vector<E> items) : items_(std::move(items)) {}
    [[nodiscard]] std::size_t size() const noexcept {
        return items_.size();
    }
    [[nodiscard]] bool empty() const noexcept {
        return items_.empty();
    }
    [[nodiscard]] const E& operator[](std::size_t i) const {
        return items_[i];
    }
    [[nodiscard]] const E& at(std::size_t i) const {
        return items_.at(i);
    }
    [[nodiscard]] auto begin() const {
        return items_.begin();
    }
    [[nodiscard]] auto end() const {
        return items_.end();
    }
    [[nodiscard]] std::optional<E> find(std::uint32_t key) const {
        for (const E& e : items_)
            if (e.key() == key)
                return e;
        return std::nullopt;
    }
    [[nodiscard]] std::optional<E> find(std::string_view object_id) const {
        for (const E& e : items_)
            if (e.object_id() == object_id)
                return e;
        return std::nullopt;
    }

private:
    std::vector<E> items_;
};

/// Object-oriented façade over the entity store.
class EntityRegistry {
public:
    explicit EntityRegistry(Client& client) : client_(&client) {}

    /// Total number of discovered entities (all domains).
    [[nodiscard]] std::size_t size() const {
        return client_->store().size();
    }

    [[nodiscard]] EntityList<BinarySensorEntity> binary_sensors() const {
        return collect<BinarySensorEntity>(EntityType::BinarySensor);
    }
    [[nodiscard]] std::optional<BinarySensorEntity> binary_sensor(const std::uint32_t key) const {
        return one<BinarySensorEntity>(EntityType::BinarySensor, client_->store().find(key));
    }
    [[nodiscard]] std::optional<BinarySensorEntity>
    binary_sensor(const std::string_view object_id) const {
        return one<BinarySensorEntity>(EntityType::BinarySensor,
                                       client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<SensorEntity> sensors() const {
        return collect<SensorEntity>(EntityType::Sensor);
    }
    [[nodiscard]] std::optional<SensorEntity> sensor(const std::uint32_t key) const {
        return one<SensorEntity>(EntityType::Sensor, client_->store().find(key));
    }
    [[nodiscard]] std::optional<SensorEntity> sensor(const std::string_view object_id) const {
        return one<SensorEntity>(EntityType::Sensor,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<TextSensorEntity> text_sensors() const {
        return collect<TextSensorEntity>(EntityType::TextSensor);
    }
    [[nodiscard]] std::optional<TextSensorEntity> text_sensor(const std::uint32_t key) const {
        return one<TextSensorEntity>(EntityType::TextSensor, client_->store().find(key));
    }
    [[nodiscard]] std::optional<TextSensorEntity>
    text_sensor(const std::string_view object_id) const {
        return one<TextSensorEntity>(EntityType::TextSensor,
                                     client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<SwitchEntity> switches() const {
        return collect<SwitchEntity>(EntityType::Switch);
    }
    // `switch` is a keyword, so the singular accessor is spelled `switch_`.
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[nodiscard]] std::optional<SwitchEntity> switch_(const std::uint32_t key) const {
        return one<SwitchEntity>(EntityType::Switch, client_->store().find(key));
    }
    // NOLINTNEXTLINE(readability-identifier-naming)
    [[nodiscard]] std::optional<SwitchEntity> switch_(const std::string_view object_id) const {
        return one<SwitchEntity>(EntityType::Switch,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<LightEntity> lights() const {
        return collect<LightEntity>(EntityType::Light);
    }
    [[nodiscard]] std::optional<LightEntity> light(const std::uint32_t key) const {
        return one<LightEntity>(EntityType::Light, client_->store().find(key));
    }
    [[nodiscard]] std::optional<LightEntity> light(const std::string_view object_id) const {
        return one<LightEntity>(EntityType::Light,
                                client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<CoverEntity> covers() const {
        return collect<CoverEntity>(EntityType::Cover);
    }
    [[nodiscard]] std::optional<CoverEntity> cover(const std::uint32_t key) const {
        return one<CoverEntity>(EntityType::Cover, client_->store().find(key));
    }
    [[nodiscard]] std::optional<CoverEntity> cover(const std::string_view object_id) const {
        return one<CoverEntity>(EntityType::Cover,
                                client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<FanEntity> fans() const {
        return collect<FanEntity>(EntityType::Fan);
    }
    [[nodiscard]] std::optional<FanEntity> fan(const std::uint32_t key) const {
        return one<FanEntity>(EntityType::Fan, client_->store().find(key));
    }
    [[nodiscard]] std::optional<FanEntity> fan(const std::string_view object_id) const {
        return one<FanEntity>(EntityType::Fan,
                              client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<ClimateEntity> climates() const {
        return collect<ClimateEntity>(EntityType::Climate);
    }
    [[nodiscard]] std::optional<ClimateEntity> climate(const std::uint32_t key) const {
        return one<ClimateEntity>(EntityType::Climate, client_->store().find(key));
    }
    [[nodiscard]] std::optional<ClimateEntity> climate(const std::string_view object_id) const {
        return one<ClimateEntity>(EntityType::Climate,
                                  client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<NumberEntity> numbers() const {
        return collect<NumberEntity>(EntityType::Number);
    }
    [[nodiscard]] std::optional<NumberEntity> number(const std::uint32_t key) const {
        return one<NumberEntity>(EntityType::Number, client_->store().find(key));
    }
    [[nodiscard]] std::optional<NumberEntity> number(const std::string_view object_id) const {
        return one<NumberEntity>(EntityType::Number,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<SelectEntity> selects() const {
        return collect<SelectEntity>(EntityType::Select);
    }
    [[nodiscard]] std::optional<SelectEntity> select(const std::uint32_t key) const {
        return one<SelectEntity>(EntityType::Select, client_->store().find(key));
    }
    [[nodiscard]] std::optional<SelectEntity> select(const std::string_view object_id) const {
        return one<SelectEntity>(EntityType::Select,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<TextEntity> texts() const {
        return collect<TextEntity>(EntityType::Text);
    }
    [[nodiscard]] std::optional<TextEntity> text(const std::uint32_t key) const {
        return one<TextEntity>(EntityType::Text, client_->store().find(key));
    }
    [[nodiscard]] std::optional<TextEntity> text(const std::string_view object_id) const {
        return one<TextEntity>(EntityType::Text,
                               client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<ButtonEntity> buttons() const {
        return collect<ButtonEntity>(EntityType::Button);
    }
    [[nodiscard]] std::optional<ButtonEntity> button(const std::uint32_t key) const {
        return one<ButtonEntity>(EntityType::Button, client_->store().find(key));
    }
    [[nodiscard]] std::optional<ButtonEntity> button(const std::string_view object_id) const {
        return one<ButtonEntity>(EntityType::Button,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<LockEntity> locks() const {
        return collect<LockEntity>(EntityType::Lock);
    }
    [[nodiscard]] std::optional<LockEntity> lock(const std::uint32_t key) const {
        return one<LockEntity>(EntityType::Lock, client_->store().find(key));
    }
    [[nodiscard]] std::optional<LockEntity> lock(const std::string_view object_id) const {
        return one<LockEntity>(EntityType::Lock,
                               client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<MediaPlayerEntity> media_players() const {
        return collect<MediaPlayerEntity>(EntityType::MediaPlayer);
    }
    [[nodiscard]] std::optional<MediaPlayerEntity> media_player(const std::uint32_t key) const {
        return one<MediaPlayerEntity>(EntityType::MediaPlayer, client_->store().find(key));
    }
    [[nodiscard]] std::optional<MediaPlayerEntity>
    media_player(const std::string_view object_id) const {
        return one<MediaPlayerEntity>(EntityType::MediaPlayer,
                                      client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<CameraEntity> cameras() const {
        return collect<CameraEntity>(EntityType::Camera);
    }
    [[nodiscard]] std::optional<CameraEntity> camera(const std::uint32_t key) const {
        return one<CameraEntity>(EntityType::Camera, client_->store().find(key));
    }
    [[nodiscard]] std::optional<CameraEntity> camera(const std::string_view object_id) const {
        return one<CameraEntity>(EntityType::Camera,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<SirenEntity> sirens() const {
        return collect<SirenEntity>(EntityType::Siren);
    }
    [[nodiscard]] std::optional<SirenEntity> siren(const std::uint32_t key) const {
        return one<SirenEntity>(EntityType::Siren, client_->store().find(key));
    }
    [[nodiscard]] std::optional<SirenEntity> siren(const std::string_view object_id) const {
        return one<SirenEntity>(EntityType::Siren,
                                client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<AlarmControlPanelEntity> alarm_control_panels() const {
        return collect<AlarmControlPanelEntity>(EntityType::AlarmControlPanel);
    }
    [[nodiscard]] std::optional<AlarmControlPanelEntity>
    alarm_control_panel(const std::uint32_t key) const {
        return one<AlarmControlPanelEntity>(EntityType::AlarmControlPanel,
                                            client_->store().find(key));
    }
    [[nodiscard]] std::optional<AlarmControlPanelEntity>
    alarm_control_panel(const std::string_view object_id) const {
        return one<AlarmControlPanelEntity>(
            EntityType::AlarmControlPanel,
            client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<DateEntity> dates() const {
        return collect<DateEntity>(EntityType::Date);
    }
    [[nodiscard]] std::optional<DateEntity> date(const std::uint32_t key) const {
        return one<DateEntity>(EntityType::Date, client_->store().find(key));
    }
    [[nodiscard]] std::optional<DateEntity> date(const std::string_view object_id) const {
        return one<DateEntity>(EntityType::Date,
                               client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<TimeEntity> times() const {
        return collect<TimeEntity>(EntityType::Time);
    }
    [[nodiscard]] std::optional<TimeEntity> time(const std::uint32_t key) const {
        return one<TimeEntity>(EntityType::Time, client_->store().find(key));
    }
    [[nodiscard]] std::optional<TimeEntity> time(const std::string_view object_id) const {
        return one<TimeEntity>(EntityType::Time,
                               client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<DateTimeEntity> datetimes() const {
        return collect<DateTimeEntity>(EntityType::DateTime);
    }
    [[nodiscard]] std::optional<DateTimeEntity> datetime(const std::uint32_t key) const {
        return one<DateTimeEntity>(EntityType::DateTime, client_->store().find(key));
    }
    [[nodiscard]] std::optional<DateTimeEntity> datetime(const std::string_view object_id) const {
        return one<DateTimeEntity>(EntityType::DateTime,
                                   client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<ValveEntity> valves() const {
        return collect<ValveEntity>(EntityType::Valve);
    }
    [[nodiscard]] std::optional<ValveEntity> valve(const std::uint32_t key) const {
        return one<ValveEntity>(EntityType::Valve, client_->store().find(key));
    }
    [[nodiscard]] std::optional<ValveEntity> valve(const std::string_view object_id) const {
        return one<ValveEntity>(EntityType::Valve,
                                client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<EventEntity> events() const {
        return collect<EventEntity>(EntityType::Event);
    }
    [[nodiscard]] std::optional<EventEntity> event(const std::uint32_t key) const {
        return one<EventEntity>(EntityType::Event, client_->store().find(key));
    }
    [[nodiscard]] std::optional<EventEntity> event(const std::string_view object_id) const {
        return one<EventEntity>(EntityType::Event,
                                client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<UpdateEntity> updates() const {
        return collect<UpdateEntity>(EntityType::Update);
    }
    [[nodiscard]] std::optional<UpdateEntity> update(const std::uint32_t key) const {
        return one<UpdateEntity>(EntityType::Update, client_->store().find(key));
    }
    [[nodiscard]] std::optional<UpdateEntity> update(const std::string_view object_id) const {
        return one<UpdateEntity>(EntityType::Update,
                                 client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<WaterHeaterEntity> water_heaters() const {
        return collect<WaterHeaterEntity>(EntityType::WaterHeater);
    }
    [[nodiscard]] std::optional<WaterHeaterEntity> water_heater(const std::uint32_t key) const {
        return one<WaterHeaterEntity>(EntityType::WaterHeater, client_->store().find(key));
    }
    [[nodiscard]] std::optional<WaterHeaterEntity>
    water_heater(const std::string_view object_id) const {
        return one<WaterHeaterEntity>(EntityType::WaterHeater,
                                      client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<InfraredEntity> infrareds() const {
        return collect<InfraredEntity>(EntityType::Infrared);
    }
    [[nodiscard]] std::optional<InfraredEntity> infrared(const std::uint32_t key) const {
        return one<InfraredEntity>(EntityType::Infrared, client_->store().find(key));
    }
    [[nodiscard]] std::optional<InfraredEntity> infrared(const std::string_view object_id) const {
        return one<InfraredEntity>(EntityType::Infrared,
                                   client_->store().find_by_object_id(std::string(object_id)));
    }
    [[nodiscard]] EntityList<RadioFrequencyEntity> radio_frequencys() const {
        return collect<RadioFrequencyEntity>(EntityType::RadioFrequency);
    }
    [[nodiscard]] std::optional<RadioFrequencyEntity>
    radio_frequency(const std::uint32_t key) const {
        return one<RadioFrequencyEntity>(EntityType::RadioFrequency, client_->store().find(key));
    }
    [[nodiscard]] std::optional<RadioFrequencyEntity>
    radio_frequency(const std::string_view object_id) const {
        return one<RadioFrequencyEntity>(
            EntityType::RadioFrequency, client_->store().find_by_object_id(std::string(object_id)));
    }

private:
    template <class E>
    EntityList<E> collect(const EntityType type) const {
        std::vector<E> out;
        for (const StoredEntity* e : client_->store().entities())
            if (e->type == type)
                out.emplace_back(*client_, e->key, e->object_id, e->name);
        return EntityList<E>(std::move(out));
    }
    template <class E>
    std::optional<E> one(const EntityType type, const StoredEntity* e) const {
        if (e == nullptr || e->type != type)
            return std::nullopt;
        return E(*client_, e->key, e->object_id, e->name);
    }
    Client* client_;
};

}  // namespace esphome::api
