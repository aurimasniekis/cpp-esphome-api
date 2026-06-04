#pragma once

/// @file
/// @brief Object-oriented entity handles.
///
/// Hierarchy: Entity (identity + client) -> TypedEntity<Derived> (common info
/// accessors) -> optionally ToggleEntity<Derived> (on/off) -> concrete handle,
/// which exposes a typed accessor for every info/state field plus command
/// helpers.

#include <esphome/api/client.hpp>
#include <esphome/api/commands/command_builder.hpp>
#include <esphome/api/model/entity_store.hpp>
#include <esphome/api/model/entity_type.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace esphome::api {

/// Common base: entity identity and the owning client.
class Entity {
public:
    Entity(Client& client, const std::uint32_t key, std::string object_id, std::string name)
        : client_(&client), key_(key), object_id_(std::move(object_id)), name_(std::move(name)) {}
    [[nodiscard]] std::uint32_t key() const noexcept {
        return key_;
    }
    [[nodiscard]] const std::string& object_id() const noexcept {
        return object_id_;
    }
    [[nodiscard]] const std::string& name() const noexcept {
        return name_;
    }

protected:
    Client* client_;
    std::uint32_t key_;
    std::string object_id_;
    std::string name_;
};

/// Adds the shared EntityInfo accessors, reading from Derived::info().
template <class Derived>
class TypedEntity : public Entity {
public:
    using Entity::Entity;
    [[nodiscard]] std::string icon() const {
        auto i = self().info();
        return i ? i->icon : std::string{};
    }
    [[nodiscard]] bool disabled_by_default() const {
        auto i = self().info();
        return i && i->disabled_by_default;
    }
    [[nodiscard]] EntityCategory entity_category() const {
        auto i = self().info();
        return i ? i->entity_category : EntityCategory::None;
    }
    [[nodiscard]] std::uint32_t device_id() const {
        auto i = self().info();
        return i ? i->device_id : 0U;
    }

private:
    const Derived& self() const {
        return *static_cast<const Derived*>(this);
    }
};

/// Adds on/off helpers. Derived must provide state() (bool `state`) and set_power(bool).
template <class Derived>
class ToggleEntity : public TypedEntity<Derived> {
public:
    using TypedEntity<Derived>::TypedEntity;
    [[nodiscard]] bool is_on() const {
        auto s = static_cast<const Derived*>(this)->state();
        return s && s->state;
    }
    void turn_on() {
        static_cast<Derived*>(this)->set_power(true);
    }
    void turn_off() {
        static_cast<Derived*>(this)->set_power(false);
    }
    void toggle() {
        static_cast<Derived*>(this)->set_power(!is_on());
    }
};

/// Handle to a BinarySensor entity.
class BinarySensorEntity : public TypedEntity<BinarySensorEntity> {
public:
    using TypedEntity<BinarySensorEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::BinarySensor;
    }
    [[nodiscard]] std::optional<BinarySensorInfo> info() const {
        return client_->store().binary_sensor_info(key_);
    }
    [[nodiscard]] std::optional<BinarySensorState> state() const {
        return client_->store().binary_sensor_state(key_);
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] bool is_status_binary_sensor() const {
        const auto i = info();
        return i ? i->is_status_binary_sensor : bool{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    /// Whether the sensor currently reads active.
    bool is_on() const {
        const auto s = state();
        return s && s->state;
    }
};

/// Handle to a Sensor entity.
class SensorEntity : public TypedEntity<SensorEntity> {
public:
    using TypedEntity<SensorEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Sensor;
    }
    [[nodiscard]] std::optional<SensorInfo> info() const {
        return client_->store().sensor_info(key_);
    }
    [[nodiscard]] std::optional<SensorState> state() const {
        return client_->store().sensor_state(key_);
    }
    [[nodiscard]] std::string unit_of_measurement() const {
        auto i = info();
        return i ? i->unit_of_measurement : std::string{};
    }
    [[nodiscard]] std::int32_t accuracy_decimals() const {
        const auto i = info();
        return i ? i->accuracy_decimals : std::int32_t{};
    }
    [[nodiscard]] bool force_update() const {
        const auto i = info();
        return i ? i->force_update : bool{};
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] SensorStateClass state_class() const {
        const auto i = info();
        return i ? i->state_class : SensorStateClass{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    /// Latest numeric value (0 if unknown).
    float value() const {
        const auto s = state();
        return s ? s->state : 0.0F;
    }
    bool has_value() const {
        const auto s = state();
        return s && !s->missing_state;
    }
};

/// Handle to a TextSensor entity.
class TextSensorEntity : public TypedEntity<TextSensorEntity> {
public:
    using TypedEntity<TextSensorEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::TextSensor;
    }
    [[nodiscard]] std::optional<TextSensorInfo> info() const {
        return client_->store().text_sensor_info(key_);
    }
    [[nodiscard]] std::optional<TextSensorState> state() const {
        return client_->store().text_sensor_state(key_);
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    std::string text() const {
        auto s = state();
        return s ? s->state : std::string{};
    }
};

/// Handle to a Switch entity.
class SwitchEntity : public ToggleEntity<SwitchEntity> {
public:
    using ToggleEntity<SwitchEntity>::ToggleEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Switch;
    }
    [[nodiscard]] std::optional<SwitchInfo> info() const {
        return client_->store().switch_info(key_);
    }
    [[nodiscard]] std::optional<SwitchState> state() const {
        return client_->store().switch_state(key_);
    }
    [[nodiscard]] bool assumed_state() const {
        const auto i = info();
        return i ? i->assumed_state : bool{};
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(SwitchCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    /// Set on/off power (used by turn_on/turn_off/toggle).
    void set_power(const bool on) const {
        send_command(*client_, SwitchCommand{key_, on});
    }
};

/// Handle to a Light entity.
class LightEntity : public ToggleEntity<LightEntity> {
public:
    using ToggleEntity<LightEntity>::ToggleEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Light;
    }
    [[nodiscard]] std::optional<LightInfo> info() const {
        return client_->store().light_info(key_);
    }
    [[nodiscard]] std::optional<LightState> state() const {
        return client_->store().light_state(key_);
    }
    [[nodiscard]] std::vector<ColorMode> supported_color_modes() const {
        auto i = info();
        return i ? i->supported_color_modes : std::vector<ColorMode>{};
    }
    [[nodiscard]] float min_mireds() const {
        const auto i = info();
        return i ? i->min_mireds : float{};
    }
    [[nodiscard]] float max_mireds() const {
        const auto i = info();
        return i ? i->max_mireds : float{};
    }
    [[nodiscard]] std::vector<std::string> effects() const {
        auto i = info();
        return i ? i->effects : std::vector<std::string>{};
    }
    [[nodiscard]] float brightness() const {
        const auto s = state();
        return s ? s->brightness : float{};
    }
    [[nodiscard]] ColorMode color_mode() const {
        const auto s = state();
        return s ? s->color_mode : ColorMode{};
    }
    [[nodiscard]] float color_brightness() const {
        const auto s = state();
        return s ? s->color_brightness : float{};
    }
    [[nodiscard]] float red() const {
        const auto s = state();
        return s ? s->red : float{};
    }
    [[nodiscard]] float green() const {
        const auto s = state();
        return s ? s->green : float{};
    }
    [[nodiscard]] float blue() const {
        const auto s = state();
        return s ? s->blue : float{};
    }
    [[nodiscard]] float white() const {
        const auto s = state();
        return s ? s->white : float{};
    }
    [[nodiscard]] float color_temperature() const {
        const auto s = state();
        return s ? s->color_temperature : float{};
    }
    [[nodiscard]] float cold_white() const {
        const auto s = state();
        return s ? s->cold_white : float{};
    }
    [[nodiscard]] float warm_white() const {
        const auto s = state();
        return s ? s->warm_white : float{};
    }
    [[nodiscard]] std::string effect() const {
        auto s = state();
        return s ? s->effect : std::string{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(LightCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    /// Set on/off power (used by turn_on/turn_off/toggle).
    void set_power(bool on) const {
        LightCommand c;
        c.key = key_;
        c.state = on;
        send_command(*client_, c);
    }
    void set_brightness(float b) const {
        LightCommand c;
        c.key = key_;
        c.state = true;
        c.brightness = b;
        send_command(*client_, c);
    }
    void set_rgb(const float r, const float g, const float b) const {
        LightCommand c;
        c.key = key_;
        c.state = true;
        c.rgb = LightRgb{r, g, b};
        send_command(*client_, c);
    }
};

/// Handle to a Cover entity.
class CoverEntity : public TypedEntity<CoverEntity> {
public:
    using TypedEntity<CoverEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Cover;
    }
    [[nodiscard]] std::optional<CoverInfo> info() const {
        return client_->store().cover_info(key_);
    }
    [[nodiscard]] std::optional<CoverState> state() const {
        return client_->store().cover_state(key_);
    }
    [[nodiscard]] bool assumed_state() const {
        const auto i = info();
        return i ? i->assumed_state : bool{};
    }
    [[nodiscard]] bool supports_position() const {
        const auto i = info();
        return i ? i->supports_position : bool{};
    }
    [[nodiscard]] bool supports_tilt() const {
        const auto i = info();
        return i ? i->supports_tilt : bool{};
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] bool supports_stop() const {
        const auto i = info();
        return i ? i->supports_stop : bool{};
    }
    [[nodiscard]] float position() const {
        const auto s = state();
        return s ? s->position : float{};
    }
    [[nodiscard]] float tilt() const {
        const auto s = state();
        return s ? s->tilt : float{};
    }
    [[nodiscard]] CoverOperation current_operation() const {
        const auto s = state();
        return s ? s->current_operation : CoverOperation{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(CoverCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void open() const {
        CoverCommand c;
        c.key = key_;
        c.position = 1.0F;
        send_command(*client_, c);
    }
    void close() const {
        CoverCommand c;
        c.key = key_;
        c.position = 0.0F;
        send_command(*client_, c);
    }
    void stop() const {
        CoverCommand c;
        c.key = key_;
        c.stop = true;
        send_command(*client_, c);
    }
    void set_position(float p) const {
        CoverCommand c;
        c.key = key_;
        c.position = p;
        send_command(*client_, c);
    }
};

/// Handle to a Fan entity.
class FanEntity : public ToggleEntity<FanEntity> {
public:
    using ToggleEntity<FanEntity>::ToggleEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Fan;
    }
    [[nodiscard]] std::optional<FanInfo> info() const {
        return client_->store().fan_info(key_);
    }
    [[nodiscard]] std::optional<FanState> state() const {
        return client_->store().fan_state(key_);
    }
    [[nodiscard]] bool supports_oscillation() const {
        const auto i = info();
        return i ? i->supports_oscillation : bool{};
    }
    [[nodiscard]] bool supports_speed() const {
        const auto i = info();
        return i ? i->supports_speed : bool{};
    }
    [[nodiscard]] bool supports_direction() const {
        const auto i = info();
        return i ? i->supports_direction : bool{};
    }
    [[nodiscard]] std::int32_t supported_speed_count() const {
        const auto i = info();
        return i ? i->supported_speed_count : std::int32_t{};
    }
    [[nodiscard]] std::vector<std::string> supported_preset_modes() const {
        auto i = info();
        return i ? i->supported_preset_modes : std::vector<std::string>{};
    }
    [[nodiscard]] bool oscillating() const {
        const auto s = state();
        return s ? s->oscillating : bool{};
    }
    [[nodiscard]] FanDirection direction() const {
        const auto s = state();
        return s ? s->direction : FanDirection{};
    }
    [[nodiscard]] std::int32_t speed_level() const {
        const auto s = state();
        return s ? s->speed_level : std::int32_t{};
    }
    [[nodiscard]] std::string preset_mode() const {
        auto s = state();
        return s ? s->preset_mode : std::string{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(FanCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    /// Set on/off power (used by turn_on/turn_off/toggle).
    void set_power(bool on) const {
        FanCommand c;
        c.key = key_;
        c.state = on;
        send_command(*client_, c);
    }
    void set_speed(std::int32_t level) const {
        FanCommand c;
        c.key = key_;
        c.speed_level = level;
        send_command(*client_, c);
    }
};

/// Handle to a Climate entity.
class ClimateEntity : public TypedEntity<ClimateEntity> {
public:
    using TypedEntity<ClimateEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Climate;
    }
    [[nodiscard]] std::optional<ClimateInfo> info() const {
        return client_->store().climate_info(key_);
    }
    [[nodiscard]] std::optional<ClimateState> state() const {
        return client_->store().climate_state(key_);
    }
    [[nodiscard]] std::vector<ClimateMode> supported_modes() const {
        auto i = info();
        return i ? i->supported_modes : std::vector<ClimateMode>{};
    }
    [[nodiscard]] float visual_min_temperature() const {
        const auto i = info();
        return i ? i->visual_min_temperature : float{};
    }
    [[nodiscard]] float visual_max_temperature() const {
        const auto i = info();
        return i ? i->visual_max_temperature : float{};
    }
    [[nodiscard]] float visual_target_temperature_step() const {
        const auto i = info();
        return i ? i->visual_target_temperature_step : float{};
    }
    [[nodiscard]] std::vector<ClimateFanMode> supported_fan_modes() const {
        auto i = info();
        return i ? i->supported_fan_modes : std::vector<ClimateFanMode>{};
    }
    [[nodiscard]] std::vector<ClimateSwingMode> supported_swing_modes() const {
        auto i = info();
        return i ? i->supported_swing_modes : std::vector<ClimateSwingMode>{};
    }
    [[nodiscard]] std::vector<std::string> supported_custom_fan_modes() const {
        auto i = info();
        return i ? i->supported_custom_fan_modes : std::vector<std::string>{};
    }
    [[nodiscard]] std::vector<ClimatePreset> supported_presets() const {
        auto i = info();
        return i ? i->supported_presets : std::vector<ClimatePreset>{};
    }
    [[nodiscard]] std::vector<std::string> supported_custom_presets() const {
        auto i = info();
        return i ? i->supported_custom_presets : std::vector<std::string>{};
    }
    [[nodiscard]] float visual_current_temperature_step() const {
        const auto i = info();
        return i ? i->visual_current_temperature_step : float{};
    }
    [[nodiscard]] float visual_min_humidity() const {
        const auto i = info();
        return i ? i->visual_min_humidity : float{};
    }
    [[nodiscard]] float visual_max_humidity() const {
        const auto i = info();
        return i ? i->visual_max_humidity : float{};
    }
    [[nodiscard]] std::uint32_t feature_flags() const {
        const auto i = info();
        return i ? i->feature_flags : std::uint32_t{};
    }
    [[nodiscard]] TemperatureUnit temperature_unit() const {
        const auto i = info();
        return i ? i->temperature_unit : TemperatureUnit{};
    }
    [[nodiscard]] ClimateMode mode() const {
        const auto s = state();
        return s ? s->mode : ClimateMode{};
    }
    [[nodiscard]] float current_temperature() const {
        const auto s = state();
        return s ? s->current_temperature : float{};
    }
    [[nodiscard]] float target_temperature() const {
        const auto s = state();
        return s ? s->target_temperature : float{};
    }
    [[nodiscard]] float target_temperature_low() const {
        const auto s = state();
        return s ? s->target_temperature_low : float{};
    }
    [[nodiscard]] float target_temperature_high() const {
        const auto s = state();
        return s ? s->target_temperature_high : float{};
    }
    [[nodiscard]] ClimateAction action() const {
        const auto s = state();
        return s ? s->action : ClimateAction{};
    }
    [[nodiscard]] ClimateFanMode fan_mode() const {
        const auto s = state();
        return s ? s->fan_mode : ClimateFanMode{};
    }
    [[nodiscard]] ClimateSwingMode swing_mode() const {
        const auto s = state();
        return s ? s->swing_mode : ClimateSwingMode{};
    }
    [[nodiscard]] std::string custom_fan_mode() const {
        auto s = state();
        return s ? s->custom_fan_mode : std::string{};
    }
    [[nodiscard]] ClimatePreset preset() const {
        const auto s = state();
        return s ? s->preset : ClimatePreset{};
    }
    [[nodiscard]] std::string custom_preset() const {
        auto s = state();
        return s ? s->custom_preset : std::string{};
    }
    [[nodiscard]] float current_humidity() const {
        const auto s = state();
        return s ? s->current_humidity : float{};
    }
    [[nodiscard]] float target_humidity() const {
        const auto s = state();
        return s ? s->target_humidity : float{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(ClimateCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void set_mode(ClimateMode m) const {
        ClimateCommand c;
        c.key = key_;
        c.mode = m;
        send_command(*client_, c);
    }
    void set_target_temperature(float t) const {
        ClimateCommand c;
        c.key = key_;
        c.target_temperature = t;
        send_command(*client_, c);
    }
};

/// Handle to a Number entity.
class NumberEntity : public TypedEntity<NumberEntity> {
public:
    using TypedEntity<NumberEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Number;
    }
    [[nodiscard]] std::optional<NumberInfo> info() const {
        return client_->store().number_info(key_);
    }
    [[nodiscard]] std::optional<NumberState> state() const {
        return client_->store().number_state(key_);
    }
    [[nodiscard]] float min_value() const {
        const auto i = info();
        return i ? i->min_value : float{};
    }
    [[nodiscard]] float max_value() const {
        const auto i = info();
        return i ? i->max_value : float{};
    }
    [[nodiscard]] float step() const {
        const auto i = info();
        return i ? i->step : float{};
    }
    [[nodiscard]] std::string unit_of_measurement() const {
        auto i = info();
        return i ? i->unit_of_measurement : std::string{};
    }
    [[nodiscard]] NumberMode mode() const {
        const auto i = info();
        return i ? i->mode : NumberMode{};
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(NumberCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void set(const float v) const {
        send_command(*client_, NumberCommand{key_, v});
    }
};

/// Handle to a Select entity.
class SelectEntity : public TypedEntity<SelectEntity> {
public:
    using TypedEntity<SelectEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Select;
    }
    [[nodiscard]] std::optional<SelectInfo> info() const {
        return client_->store().select_info(key_);
    }
    [[nodiscard]] std::optional<SelectState> state() const {
        return client_->store().select_state(key_);
    }
    [[nodiscard]] std::vector<std::string> options() const {
        auto i = info();
        return i ? i->options : std::vector<std::string>{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(SelectCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void set(const std::string& v) const {
        send_command(*client_, SelectCommand{key_, v});
    }
};

/// Handle to a Text entity.
class TextEntity : public TypedEntity<TextEntity> {
public:
    using TypedEntity<TextEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Text;
    }
    [[nodiscard]] std::optional<TextInfo> info() const {
        return client_->store().text_info(key_);
    }
    [[nodiscard]] std::optional<TextState> state() const {
        return client_->store().text_state(key_);
    }
    [[nodiscard]] std::uint32_t min_length() const {
        const auto i = info();
        return i ? i->min_length : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t max_length() const {
        const auto i = info();
        return i ? i->max_length : std::uint32_t{};
    }
    [[nodiscard]] std::string pattern() const {
        auto i = info();
        return i ? i->pattern : std::string{};
    }
    [[nodiscard]] TextMode mode() const {
        const auto i = info();
        return i ? i->mode : TextMode{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(TextCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void set(const std::string& v) const {
        send_command(*client_, TextCommand{key_, v});
    }
};

/// Handle to a Button entity.
class ButtonEntity : public TypedEntity<ButtonEntity> {
public:
    using TypedEntity<ButtonEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Button;
    }
    [[nodiscard]] std::optional<ButtonInfo> info() const {
        return client_->store().button_info(key_);
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(ButtonCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void press() const {
        send_command(*client_, ButtonCommand{key_});
    }
};

/// Handle to a Lock entity.
class LockEntity : public TypedEntity<LockEntity> {
public:
    using TypedEntity<LockEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Lock;
    }
    [[nodiscard]] std::optional<LockInfo> info() const {
        return client_->store().lock_info(key_);
    }
    [[nodiscard]] std::optional<LockEntityState> state() const {
        return client_->store().lock_state(key_);
    }
    [[nodiscard]] bool assumed_state() const {
        const auto i = info();
        return i ? i->assumed_state : bool{};
    }
    [[nodiscard]] bool supports_open() const {
        const auto i = info();
        return i ? i->supports_open : bool{};
    }
    [[nodiscard]] bool requires_code() const {
        const auto i = info();
        return i ? i->requires_code : bool{};
    }
    [[nodiscard]] std::string code_format() const {
        auto i = info();
        return i ? i->code_format : std::string{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(LockCommandData c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void lock() const {
        LockCommandData c;
        c.key = key_;
        c.command = LockCommand::Lock;
        send_command(*client_, c);
    }
    void unlock() const {
        LockCommandData c;
        c.key = key_;
        c.command = LockCommand::Unlock;
        send_command(*client_, c);
    }
    void open() const {
        LockCommandData c;
        c.key = key_;
        c.command = LockCommand::Open;
        send_command(*client_, c);
    }
};

/// Handle to a MediaPlayer entity.
class MediaPlayerEntity : public TypedEntity<MediaPlayerEntity> {
public:
    using TypedEntity<MediaPlayerEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::MediaPlayer;
    }
    [[nodiscard]] std::optional<MediaPlayerInfo> info() const {
        return client_->store().media_player_info(key_);
    }
    [[nodiscard]] std::optional<MediaPlayerStatus> state() const {
        return client_->store().media_player_state(key_);
    }
    [[nodiscard]] bool supports_pause() const {
        const auto i = info();
        return i ? i->supports_pause : bool{};
    }
    [[nodiscard]] std::vector<MediaPlayerSupportedFormat> supported_formats() const {
        auto i = info();
        return i ? i->supported_formats : std::vector<MediaPlayerSupportedFormat>{};
    }
    [[nodiscard]] float volume() const {
        const auto s = state();
        return s ? s->volume : float{};
    }
    [[nodiscard]] bool muted() const {
        const auto s = state();
        return s ? s->muted : bool{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(MediaPlayerControl c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void play() const {
        MediaPlayerControl c;
        c.key = key_;
        c.command = MediaPlayerCommand::Play;
        send_command(*client_, c);
    }
    void pause() const {
        MediaPlayerControl c;
        c.key = key_;
        c.command = MediaPlayerCommand::Pause;
        send_command(*client_, c);
    }
    void stop() const {
        MediaPlayerControl c;
        c.key = key_;
        c.command = MediaPlayerCommand::Stop;
        send_command(*client_, c);
    }
    void set_volume(float v) const {
        MediaPlayerControl c;
        c.key = key_;
        c.volume = v;
        send_command(*client_, c);
    }
};

/// Handle to a Camera entity.
class CameraEntity : public TypedEntity<CameraEntity> {
public:
    using TypedEntity<CameraEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Camera;
    }
    [[nodiscard]] std::optional<CameraInfo> info() const {
        return client_->store().camera_info(key_);
    }
    /// Request a still image (single) or start a stream.
    void request_image(const bool single = true, const bool stream = false) const {
        request_camera_image(*client_, single, stream);
    }
};

/// Handle to a Siren entity.
class SirenEntity : public ToggleEntity<SirenEntity> {
public:
    using ToggleEntity<SirenEntity>::ToggleEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Siren;
    }
    [[nodiscard]] std::optional<SirenInfo> info() const {
        return client_->store().siren_info(key_);
    }
    [[nodiscard]] std::optional<SirenState> state() const {
        return client_->store().siren_state(key_);
    }
    [[nodiscard]] std::vector<std::string> tones() const {
        auto i = info();
        return i ? i->tones : std::vector<std::string>{};
    }
    [[nodiscard]] bool supports_duration() const {
        const auto i = info();
        return i ? i->supports_duration : bool{};
    }
    [[nodiscard]] bool supports_volume() const {
        const auto i = info();
        return i ? i->supports_volume : bool{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(SirenCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    /// Set on/off power (used by turn_on/turn_off/toggle).
    void set_power(bool on) const {
        SirenCommand c;
        c.key = key_;
        c.state = on;
        send_command(*client_, c);
    }
};

/// Handle to a AlarmControlPanel entity.
class AlarmControlPanelEntity : public TypedEntity<AlarmControlPanelEntity> {
public:
    using TypedEntity<AlarmControlPanelEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::AlarmControlPanel;
    }
    [[nodiscard]] std::optional<AlarmControlPanelInfo> info() const {
        return client_->store().alarm_control_panel_info(key_);
    }
    [[nodiscard]] std::optional<AlarmControlPanelStatus> state() const {
        return client_->store().alarm_control_panel_state(key_);
    }
    [[nodiscard]] std::uint32_t supported_features() const {
        const auto i = info();
        return i ? i->supported_features : std::uint32_t{};
    }
    [[nodiscard]] bool requires_code() const {
        const auto i = info();
        return i ? i->requires_code : bool{};
    }
    [[nodiscard]] bool requires_code_to_arm() const {
        const auto i = info();
        return i ? i->requires_code_to_arm : bool{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(AlarmControlPanelCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void disarm(const std::string& code = {}) const {
        send_command(*client_,
                     AlarmControlPanelCommand{key_, AlarmControlPanelStateCommand::Disarm, code});
    }
    void arm_away(const std::string& code = {}) const {
        send_command(*client_,
                     AlarmControlPanelCommand{key_, AlarmControlPanelStateCommand::ArmAway, code});
    }
    void arm_home(const std::string& code = {}) const {
        send_command(*client_,
                     AlarmControlPanelCommand{key_, AlarmControlPanelStateCommand::ArmHome, code});
    }
};

/// Handle to a Date entity.
class DateEntity : public TypedEntity<DateEntity> {
public:
    using TypedEntity<DateEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Date;
    }
    [[nodiscard]] std::optional<DateInfo> info() const {
        return client_->store().date_info(key_);
    }
    [[nodiscard]] std::optional<DateState> state() const {
        return client_->store().date_state(key_);
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    [[nodiscard]] std::uint32_t year() const {
        const auto s = state();
        return s ? s->year : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t month() const {
        const auto s = state();
        return s ? s->month : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t day() const {
        const auto s = state();
        return s ? s->day : std::uint32_t{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(DateCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void set(const std::uint32_t year, const std::uint32_t month, const std::uint32_t day) const {
        send_command(*client_, DateCommand{key_, year, month, day});
    }
};

/// Handle to a Time entity.
class TimeEntity : public TypedEntity<TimeEntity> {
public:
    using TypedEntity<TimeEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Time;
    }
    [[nodiscard]] std::optional<TimeInfo> info() const {
        return client_->store().time_info(key_);
    }
    [[nodiscard]] std::optional<TimeState> state() const {
        return client_->store().time_state(key_);
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    [[nodiscard]] std::uint32_t hour() const {
        const auto s = state();
        return s ? s->hour : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t minute() const {
        const auto s = state();
        return s ? s->minute : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t second() const {
        const auto s = state();
        return s ? s->second : std::uint32_t{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(TimeCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void
    set(const std::uint32_t hour, const std::uint32_t minute, const std::uint32_t second) const {
        send_command(*client_, TimeCommand{key_, hour, minute, second});
    }
};

/// Handle to a DateTime entity.
class DateTimeEntity : public TypedEntity<DateTimeEntity> {
public:
    using TypedEntity<DateTimeEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::DateTime;
    }
    [[nodiscard]] std::optional<DateTimeInfo> info() const {
        return client_->store().datetime_info(key_);
    }
    [[nodiscard]] std::optional<DateTimeState> state() const {
        return client_->store().datetime_state(key_);
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    [[nodiscard]] std::uint32_t epoch_seconds() const {
        const auto s = state();
        return s ? s->epoch_seconds : std::uint32_t{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(DateTimeCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void set(const std::uint32_t epoch_seconds) const {
        send_command(*client_, DateTimeCommand{key_, epoch_seconds});
    }
};

/// Handle to a Valve entity.
class ValveEntity : public TypedEntity<ValveEntity> {
public:
    using TypedEntity<ValveEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Valve;
    }
    [[nodiscard]] std::optional<ValveInfo> info() const {
        return client_->store().valve_info(key_);
    }
    [[nodiscard]] std::optional<ValveState> state() const {
        return client_->store().valve_state(key_);
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] bool assumed_state() const {
        const auto i = info();
        return i ? i->assumed_state : bool{};
    }
    [[nodiscard]] bool supports_position() const {
        const auto i = info();
        return i ? i->supports_position : bool{};
    }
    [[nodiscard]] bool supports_stop() const {
        const auto i = info();
        return i ? i->supports_stop : bool{};
    }
    [[nodiscard]] float position() const {
        const auto s = state();
        return s ? s->position : float{};
    }
    [[nodiscard]] ValveOperation current_operation() const {
        const auto s = state();
        return s ? s->current_operation : ValveOperation{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(ValveCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void open() const {
        ValveCommand c;
        c.key = key_;
        c.position = 1.0F;
        send_command(*client_, c);
    }
    void close() const {
        ValveCommand c;
        c.key = key_;
        c.position = 0.0F;
        send_command(*client_, c);
    }
    void stop() const {
        ValveCommand c;
        c.key = key_;
        c.stop = true;
        send_command(*client_, c);
    }
};

/// Handle to a Event entity.
class EventEntity : public TypedEntity<EventEntity> {
public:
    using TypedEntity<EventEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Event;
    }
    [[nodiscard]] std::optional<EventInfo> info() const {
        return client_->store().event_info(key_);
    }
    [[nodiscard]] std::optional<EventState> state() const {
        return client_->store().event_state(key_);
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] std::vector<std::string> event_types() const {
        auto i = info();
        return i ? i->event_types : std::vector<std::string>{};
    }
    [[nodiscard]] std::string event_type() const {
        auto s = state();
        return s ? s->event_type : std::string{};
    }
};

/// Handle to a Update entity.
class UpdateEntity : public TypedEntity<UpdateEntity> {
public:
    using TypedEntity<UpdateEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Update;
    }
    [[nodiscard]] std::optional<UpdateInfo> info() const {
        return client_->store().update_info(key_);
    }
    [[nodiscard]] std::optional<UpdateState> state() const {
        return client_->store().update_state(key_);
    }
    [[nodiscard]] std::string device_class() const {
        auto i = info();
        return i ? i->device_class : std::string{};
    }
    [[nodiscard]] bool missing_state() const {
        const auto s = state();
        return s ? s->missing_state : bool{};
    }
    [[nodiscard]] bool in_progress() const {
        const auto s = state();
        return s ? s->in_progress : bool{};
    }
    [[nodiscard]] bool has_progress() const {
        const auto s = state();
        return s ? s->has_progress : bool{};
    }
    [[nodiscard]] float progress() const {
        const auto s = state();
        return s ? s->progress : float{};
    }
    [[nodiscard]] std::string current_version() const {
        auto s = state();
        return s ? s->current_version : std::string{};
    }
    [[nodiscard]] std::string latest_version() const {
        auto s = state();
        return s ? s->latest_version : std::string{};
    }
    [[nodiscard]] std::string title() const {
        auto s = state();
        return s ? s->title : std::string{};
    }
    [[nodiscard]] std::string release_summary() const {
        auto s = state();
        return s ? s->release_summary : std::string{};
    }
    [[nodiscard]] std::string release_url() const {
        auto s = state();
        return s ? s->release_url : std::string{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(UpdateControl c) const {
        c.key = key_;
        send_command(*client_, c);
    }
    void install() const {
        send_command(*client_, UpdateControl{key_, UpdateCommand::Update});
    }
    void check() const {
        send_command(*client_, UpdateControl{key_, UpdateCommand::Check});
    }
};

/// Handle to a WaterHeater entity.
class WaterHeaterEntity : public TypedEntity<WaterHeaterEntity> {
public:
    using TypedEntity<WaterHeaterEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::WaterHeater;
    }
    [[nodiscard]] std::optional<WaterHeaterInfo> info() const {
        return client_->store().water_heater_info(key_);
    }
    [[nodiscard]] std::optional<WaterHeaterState> state() const {
        return client_->store().water_heater_state(key_);
    }
    [[nodiscard]] float min_temperature() const {
        const auto i = info();
        return i ? i->min_temperature : float{};
    }
    [[nodiscard]] float max_temperature() const {
        const auto i = info();
        return i ? i->max_temperature : float{};
    }
    [[nodiscard]] float target_temperature_step() const {
        const auto i = info();
        return i ? i->target_temperature_step : float{};
    }
    [[nodiscard]] std::vector<WaterHeaterMode> supported_modes() const {
        auto i = info();
        return i ? i->supported_modes : std::vector<WaterHeaterMode>{};
    }
    [[nodiscard]] std::uint32_t supported_features() const {
        const auto i = info();
        return i ? i->supported_features : std::uint32_t{};
    }
    [[nodiscard]] TemperatureUnit temperature_unit() const {
        const auto i = info();
        return i ? i->temperature_unit : TemperatureUnit{};
    }
    [[nodiscard]] float current_temperature() const {
        const auto s = state();
        return s ? s->current_temperature : float{};
    }
    [[nodiscard]] float target_temperature() const {
        const auto s = state();
        return s ? s->target_temperature : float{};
    }
    [[nodiscard]] WaterHeaterMode mode() const {
        const auto s = state();
        return s ? s->mode : WaterHeaterMode{};
    }
    [[nodiscard]] float target_temperature_low() const {
        const auto s = state();
        return s ? s->target_temperature_low : float{};
    }
    [[nodiscard]] float target_temperature_high() const {
        const auto s = state();
        return s ? s->target_temperature_high : float{};
    }
    /// Send a fully-specified command (its key is set automatically).
    void command(WaterHeaterCommand c) const {
        c.key = key_;
        send_command(*client_, c);
    }
};

/// Handle to a Infrared entity.
class InfraredEntity : public TypedEntity<InfraredEntity> {
public:
    using TypedEntity<InfraredEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::Infrared;
    }
    [[nodiscard]] std::optional<InfraredInfo> info() const {
        return client_->store().infrared_info(key_);
    }
    [[nodiscard]] std::uint32_t capabilities() const {
        const auto i = info();
        return i ? i->capabilities : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t receiver_frequency() const {
        const auto i = info();
        return i ? i->receiver_frequency : std::uint32_t{};
    }
};

/// Handle to a RadioFrequency entity.
class RadioFrequencyEntity : public TypedEntity<RadioFrequencyEntity> {
public:
    using TypedEntity<RadioFrequencyEntity>::TypedEntity;
    [[nodiscard]] static constexpr EntityType type() {
        return EntityType::RadioFrequency;
    }
    [[nodiscard]] std::optional<RadioFrequencyInfo> info() const {
        return client_->store().radio_frequency_info(key_);
    }
    [[nodiscard]] std::uint32_t capabilities() const {
        const auto i = info();
        return i ? i->capabilities : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t frequency_min() const {
        const auto i = info();
        return i ? i->frequency_min : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t frequency_max() const {
        const auto i = info();
        return i ? i->frequency_max : std::uint32_t{};
    }
    [[nodiscard]] std::uint32_t supported_modulations() const {
        const auto i = info();
        return i ? i->supported_modulations : std::uint32_t{};
    }
};

}  // namespace esphome::api
