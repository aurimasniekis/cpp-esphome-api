#pragma once

/// @file
/// @brief Optional JSON (de)serialization for the public value types.
///
/// This header is **opt-in** and dependency-light: it only does anything when
/// <nlohmann/json.hpp> is available (auto-detected via `__has_include`, or
/// forced with ESPHOME_API_WITH_JSON). It is intentionally **not** pulled in by
/// `<esphome/api/api.hpp>`, so the core library never depends on nlohmann::json
/// — include this header explicitly from a consumer (e.g. a CLI) that already
/// links nlohmann::json to get ADL `to_json` / `from_json` for every type.
///
/// Only `std::optional`-free value types are covered here (all the info, state,
/// device-info, discovery, log, Bluetooth and serial-proxy types). The command
/// structs carry `std::optional` fields and are intentionally left out: a
/// consumer that needs them can supply its own `adl_serializer<std::optional>`.

#if defined(ESPHOME_API_WITH_JSON) || defined(__has_include)
#if defined(ESPHOME_API_WITH_JSON) || __has_include(<nlohmann/json.hpp>)
#define ESPHOME_API_JSON_AVAILABLE 1
#endif
#endif

#ifdef ESPHOME_API_JSON_AVAILABLE

#include <esphome/api/discovery.hpp>
#include <esphome/api/model/device_info.hpp>
#include <esphome/api/model/entities/lock.hpp>
#include <esphome/api/model/entity_store.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/bluetooth_proxy.hpp>
#include <esphome/api/subsystems/log_stream.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>

#include <nlohmann/json.hpp>

namespace esphome::api {

// ---------------------------------------------------------------------------
// Enums — serialized as their lower_snake_case names (first entry is the
// fallback used for unknown values on read).
// ---------------------------------------------------------------------------

// The enum (de)serializers below expand to a static C-style lookup array inside
// each function; that is nlohmann's macro, not our style choice.
// NOLINTBEGIN(cert-err58-cpp,modernize-avoid-c-arrays,modernize-type-traits)
NLOHMANN_JSON_SERIALIZE_ENUM(EntityCategory,
                             {
                                 {EntityCategory::None, "none"},
                                 {EntityCategory::Config, "config"},
                                 {EntityCategory::Diagnostic, "diagnostic"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ColorMode,
                             {
                                 {ColorMode::Unknown, "unknown"},
                                 {ColorMode::OnOff, "on_off"},
                                 {ColorMode::LegacyBrightness, "legacy_brightness"},
                                 {ColorMode::Brightness, "brightness"},
                                 {ColorMode::White, "white"},
                                 {ColorMode::ColorTemperature, "color_temperature"},
                                 {ColorMode::ColdWarmWhite, "cold_warm_white"},
                                 {ColorMode::Rgb, "rgb"},
                                 {ColorMode::RgbWhite, "rgb_white"},
                                 {ColorMode::RgbColorTemperature, "rgb_color_temperature"},
                                 {ColorMode::RgbColdWarmWhite, "rgb_cold_warm_white"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(CoverOperation,
                             {
                                 {CoverOperation::Idle, "idle"},
                                 {CoverOperation::IsOpening, "is_opening"},
                                 {CoverOperation::IsClosing, "is_closing"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ValveOperation,
                             {
                                 {ValveOperation::Idle, "idle"},
                                 {ValveOperation::IsOpening, "is_opening"},
                                 {ValveOperation::IsClosing, "is_closing"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(FanDirection,
                             {
                                 {FanDirection::Forward, "forward"},
                                 {FanDirection::Reverse, "reverse"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(SensorStateClass,
                             {
                                 {SensorStateClass::None, "none"},
                                 {SensorStateClass::Measurement, "measurement"},
                                 {SensorStateClass::TotalIncreasing, "total_increasing"},
                                 {SensorStateClass::Total, "total"},
                                 {SensorStateClass::MeasurementAngle, "measurement_angle"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(LogLevel,
                             {
                                 {LogLevel::None, "none"},
                                 {LogLevel::Error, "error"},
                                 {LogLevel::Warn, "warn"},
                                 {LogLevel::Info, "info"},
                                 {LogLevel::Config, "config"},
                                 {LogLevel::Debug, "debug"},
                                 {LogLevel::Verbose, "verbose"},
                                 {LogLevel::VeryVerbose, "very_verbose"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(TemperatureUnit,
                             {
                                 {TemperatureUnit::Celsius, "celsius"},
                                 {TemperatureUnit::Fahrenheit, "fahrenheit"},
                                 {TemperatureUnit::Kelvin, "kelvin"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ClimateMode,
                             {
                                 {ClimateMode::Off, "off"},
                                 {ClimateMode::HeatCool, "heat_cool"},
                                 {ClimateMode::Cool, "cool"},
                                 {ClimateMode::Heat, "heat"},
                                 {ClimateMode::FanOnly, "fan_only"},
                                 {ClimateMode::Dry, "dry"},
                                 {ClimateMode::Auto, "auto"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ClimateFanMode,
                             {
                                 {ClimateFanMode::On, "on"},
                                 {ClimateFanMode::Off, "off"},
                                 {ClimateFanMode::Auto, "auto"},
                                 {ClimateFanMode::Low, "low"},
                                 {ClimateFanMode::Medium, "medium"},
                                 {ClimateFanMode::High, "high"},
                                 {ClimateFanMode::Middle, "middle"},
                                 {ClimateFanMode::Focus, "focus"},
                                 {ClimateFanMode::Diffuse, "diffuse"},
                                 {ClimateFanMode::Quiet, "quiet"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ClimateSwingMode,
                             {
                                 {ClimateSwingMode::Off, "off"},
                                 {ClimateSwingMode::Both, "both"},
                                 {ClimateSwingMode::Vertical, "vertical"},
                                 {ClimateSwingMode::Horizontal, "horizontal"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ClimateAction,
                             {
                                 {ClimateAction::Off, "off"},
                                 {ClimateAction::Cooling, "cooling"},
                                 {ClimateAction::Heating, "heating"},
                                 {ClimateAction::Idle, "idle"},
                                 {ClimateAction::Drying, "drying"},
                                 {ClimateAction::Fan, "fan"},
                                 {ClimateAction::Defrosting, "defrosting"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(ClimatePreset,
                             {
                                 {ClimatePreset::None, "none"},
                                 {ClimatePreset::Home, "home"},
                                 {ClimatePreset::Away, "away"},
                                 {ClimatePreset::Boost, "boost"},
                                 {ClimatePreset::Comfort, "comfort"},
                                 {ClimatePreset::Eco, "eco"},
                                 {ClimatePreset::Sleep, "sleep"},
                                 {ClimatePreset::Activity, "activity"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(NumberMode,
                             {
                                 {NumberMode::Auto, "auto"},
                                 {NumberMode::Box, "box"},
                                 {NumberMode::Slider, "slider"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(TextMode,
                             {
                                 {TextMode::Text, "text"},
                                 {TextMode::Password, "password"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(LockState,
                             {
                                 {LockState::None, "none"},
                                 {LockState::Locked, "locked"},
                                 {LockState::Unlocked, "unlocked"},
                                 {LockState::Jammed, "jammed"},
                                 {LockState::Locking, "locking"},
                                 {LockState::Unlocking, "unlocking"},
                                 {LockState::Opening, "opening"},
                                 {LockState::Open, "open"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(MediaPlayerState,
                             {
                                 {MediaPlayerState::None, "none"},
                                 {MediaPlayerState::Idle, "idle"},
                                 {MediaPlayerState::Playing, "playing"},
                                 {MediaPlayerState::Paused, "paused"},
                                 {MediaPlayerState::Announcing, "announcing"},
                                 {MediaPlayerState::Off, "off"},
                                 {MediaPlayerState::On, "on"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(MediaPlayerFormatPurpose,
                             {
                                 {MediaPlayerFormatPurpose::Default, "default"},
                                 {MediaPlayerFormatPurpose::Announcement, "announcement"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(AlarmControlPanelState,
                             {
                                 {AlarmControlPanelState::Disarmed, "disarmed"},
                                 {AlarmControlPanelState::ArmedHome, "armed_home"},
                                 {AlarmControlPanelState::ArmedAway, "armed_away"},
                                 {AlarmControlPanelState::ArmedNight, "armed_night"},
                                 {AlarmControlPanelState::ArmedVacation, "armed_vacation"},
                                 {AlarmControlPanelState::ArmedCustomBypass, "armed_custom_bypass"},
                                 {AlarmControlPanelState::Pending, "pending"},
                                 {AlarmControlPanelState::Arming, "arming"},
                                 {AlarmControlPanelState::Disarming, "disarming"},
                                 {AlarmControlPanelState::Triggered, "triggered"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(WaterHeaterMode,
                             {
                                 {WaterHeaterMode::Off, "off"},
                                 {WaterHeaterMode::Eco, "eco"},
                                 {WaterHeaterMode::Electric, "electric"},
                                 {WaterHeaterMode::Performance, "performance"},
                                 {WaterHeaterMode::HighDemand, "high_demand"},
                                 {WaterHeaterMode::HeatPump, "heat_pump"},
                                 {WaterHeaterMode::Gas, "gas"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(LockCommand,
                             {
                                 {LockCommand::Unlock, "unlock"},
                                 {LockCommand::Lock, "lock"},
                                 {LockCommand::Open, "open"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(SerialProxyPortType,
                             {
                                 {SerialProxyPortType::Ttl, "ttl"},
                                 {SerialProxyPortType::Rs232, "rs232"},
                                 {SerialProxyPortType::Rs485, "rs485"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(SerialProxyParity,
                             {
                                 {SerialProxyParity::None, "none"},
                                 {SerialProxyParity::Even, "even"},
                                 {SerialProxyParity::Odd, "odd"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(SerialProxyRequestType,
                             {
                                 {SerialProxyRequestType::Subscribe, "subscribe"},
                                 {SerialProxyRequestType::Unsubscribe, "unsubscribe"},
                                 {SerialProxyRequestType::Flush, "flush"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(SerialProxyStatus,
                             {
                                 {SerialProxyStatus::Ok, "ok"},
                                 {SerialProxyStatus::AssumedSuccess, "assumed_success"},
                                 {SerialProxyStatus::Error, "error"},
                                 {SerialProxyStatus::Timeout, "timeout"},
                                 {SerialProxyStatus::NotSupported, "not_supported"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(BluetoothScannerState,
                             {
                                 {BluetoothScannerState::Idle, "idle"},
                                 {BluetoothScannerState::Starting, "starting"},
                                 {BluetoothScannerState::Running, "running"},
                                 {BluetoothScannerState::Failed, "failed"},
                                 {BluetoothScannerState::Stopping, "stopping"},
                                 {BluetoothScannerState::Stopped, "stopped"},
                             })

NLOHMANN_JSON_SERIALIZE_ENUM(BluetoothScannerMode,
                             {
                                 {BluetoothScannerMode::Passive, "passive"},
                                 {BluetoothScannerMode::Active, "active"},
                             })
// NOLINTEND(cert-err58-cpp,modernize-avoid-c-arrays,modernize-type-traits)

// ---------------------------------------------------------------------------
// Shared entity metadata (base of every <X>Info).
// ---------------------------------------------------------------------------

inline void entity_info_to_json(nlohmann::json& j, const EntityInfo& v) {
    j["key"] = v.key;
    j["object_id"] = v.object_id;
    j["name"] = v.name;
    j["icon"] = v.icon;
    j["disabled_by_default"] = v.disabled_by_default;
    j["entity_category"] = v.entity_category;
    j["device_id"] = v.device_id;
}

inline void entity_info_from_json(const nlohmann::json& j, EntityInfo& v) {
    v.key = j.value("key", 0U);
    v.object_id = j.value("object_id", std::string{});
    v.name = j.value("name", std::string{});
    v.icon = j.value("icon", std::string{});
    v.disabled_by_default = j.value("disabled_by_default", false);
    v.entity_category = j.value("entity_category", EntityCategory::None);
    v.device_id = j.value("device_id", 0U);
}

// ---------------------------------------------------------------------------
// Entity info structs.
// ---------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const BinarySensorInfo& v) {
    entity_info_to_json(j, v);
    j["device_class"] = v.device_class;
    j["is_status_binary_sensor"] = v.is_status_binary_sensor;
}
inline void from_json(const nlohmann::json& j, BinarySensorInfo& v) {
    entity_info_from_json(j, v);
    v.device_class = j.value("device_class", std::string{});
    v.is_status_binary_sensor = j.value("is_status_binary_sensor", false);
}

inline void to_json(nlohmann::json& j, const SensorInfo& v) {
    entity_info_to_json(j, v);
    j["unit_of_measurement"] = v.unit_of_measurement;
    j["accuracy_decimals"] = v.accuracy_decimals;
    j["force_update"] = v.force_update;
    j["device_class"] = v.device_class;
    j["state_class"] = v.state_class;
}
inline void from_json(const nlohmann::json& j, SensorInfo& v) {
    entity_info_from_json(j, v);
    v.unit_of_measurement = j.value("unit_of_measurement", std::string{});
    v.accuracy_decimals = j.value("accuracy_decimals", 0);
    v.force_update = j.value("force_update", false);
    v.device_class = j.value("device_class", std::string{});
    v.state_class = j.value("state_class", SensorStateClass::None);
}

inline void to_json(nlohmann::json& j, const TextSensorInfo& v) {
    entity_info_to_json(j, v);
    j["device_class"] = v.device_class;
}
inline void from_json(const nlohmann::json& j, TextSensorInfo& v) {
    entity_info_from_json(j, v);
    v.device_class = j.value("device_class", std::string{});
}

inline void to_json(nlohmann::json& j, const SwitchInfo& v) {
    entity_info_to_json(j, v);
    j["assumed_state"] = v.assumed_state;
    j["device_class"] = v.device_class;
}
inline void from_json(const nlohmann::json& j, SwitchInfo& v) {
    entity_info_from_json(j, v);
    v.assumed_state = j.value("assumed_state", false);
    v.device_class = j.value("device_class", std::string{});
}

inline void to_json(nlohmann::json& j, const LightInfo& v) {
    entity_info_to_json(j, v);
    j["supported_color_modes"] = v.supported_color_modes;
    j["min_mireds"] = v.min_mireds;
    j["max_mireds"] = v.max_mireds;
    j["effects"] = v.effects;
}
inline void from_json(const nlohmann::json& j, LightInfo& v) {
    entity_info_from_json(j, v);
    v.supported_color_modes = j.value("supported_color_modes", std::vector<ColorMode>{});
    v.min_mireds = j.value("min_mireds", 0.0F);
    v.max_mireds = j.value("max_mireds", 0.0F);
    v.effects = j.value("effects", std::vector<std::string>{});
}

inline void to_json(nlohmann::json& j, const CoverInfo& v) {
    entity_info_to_json(j, v);
    j["assumed_state"] = v.assumed_state;
    j["supports_position"] = v.supports_position;
    j["supports_tilt"] = v.supports_tilt;
    j["device_class"] = v.device_class;
    j["supports_stop"] = v.supports_stop;
}
inline void from_json(const nlohmann::json& j, CoverInfo& v) {
    entity_info_from_json(j, v);
    v.assumed_state = j.value("assumed_state", false);
    v.supports_position = j.value("supports_position", false);
    v.supports_tilt = j.value("supports_tilt", false);
    v.device_class = j.value("device_class", std::string{});
    v.supports_stop = j.value("supports_stop", false);
}

inline void to_json(nlohmann::json& j, const FanInfo& v) {
    entity_info_to_json(j, v);
    j["supports_oscillation"] = v.supports_oscillation;
    j["supports_speed"] = v.supports_speed;
    j["supports_direction"] = v.supports_direction;
    j["supported_speed_count"] = v.supported_speed_count;
    j["supported_preset_modes"] = v.supported_preset_modes;
}
inline void from_json(const nlohmann::json& j, FanInfo& v) {
    entity_info_from_json(j, v);
    v.supports_oscillation = j.value("supports_oscillation", false);
    v.supports_speed = j.value("supports_speed", false);
    v.supports_direction = j.value("supports_direction", false);
    v.supported_speed_count = j.value("supported_speed_count", 0);
    v.supported_preset_modes = j.value("supported_preset_modes", std::vector<std::string>{});
}

inline void to_json(nlohmann::json& j, const ClimateInfo& v) {
    entity_info_to_json(j, v);
    j["supported_modes"] = v.supported_modes;
    j["visual_min_temperature"] = v.visual_min_temperature;
    j["visual_max_temperature"] = v.visual_max_temperature;
    j["visual_target_temperature_step"] = v.visual_target_temperature_step;
    j["supported_fan_modes"] = v.supported_fan_modes;
    j["supported_swing_modes"] = v.supported_swing_modes;
    j["supported_custom_fan_modes"] = v.supported_custom_fan_modes;
    j["supported_presets"] = v.supported_presets;
    j["supported_custom_presets"] = v.supported_custom_presets;
    j["visual_current_temperature_step"] = v.visual_current_temperature_step;
    j["visual_min_humidity"] = v.visual_min_humidity;
    j["visual_max_humidity"] = v.visual_max_humidity;
    j["feature_flags"] = v.feature_flags;
    j["temperature_unit"] = v.temperature_unit;
}
inline void from_json(const nlohmann::json& j, ClimateInfo& v) {
    entity_info_from_json(j, v);
    v.supported_modes = j.value("supported_modes", std::vector<ClimateMode>{});
    v.visual_min_temperature = j.value("visual_min_temperature", 0.0F);
    v.visual_max_temperature = j.value("visual_max_temperature", 0.0F);
    v.visual_target_temperature_step = j.value("visual_target_temperature_step", 0.0F);
    v.supported_fan_modes = j.value("supported_fan_modes", std::vector<ClimateFanMode>{});
    v.supported_swing_modes = j.value("supported_swing_modes", std::vector<ClimateSwingMode>{});
    v.supported_custom_fan_modes =
        j.value("supported_custom_fan_modes", std::vector<std::string>{});
    v.supported_presets = j.value("supported_presets", std::vector<ClimatePreset>{});
    v.supported_custom_presets = j.value("supported_custom_presets", std::vector<std::string>{});
    v.visual_current_temperature_step = j.value("visual_current_temperature_step", 0.0F);
    v.visual_min_humidity = j.value("visual_min_humidity", 0.0F);
    v.visual_max_humidity = j.value("visual_max_humidity", 0.0F);
    v.feature_flags = j.value("feature_flags", 0U);
    v.temperature_unit = j.value("temperature_unit", TemperatureUnit::Celsius);
}

inline void to_json(nlohmann::json& j, const NumberInfo& v) {
    entity_info_to_json(j, v);
    j["min_value"] = v.min_value;
    j["max_value"] = v.max_value;
    j["step"] = v.step;
    j["unit_of_measurement"] = v.unit_of_measurement;
    j["mode"] = v.mode;
    j["device_class"] = v.device_class;
}
inline void from_json(const nlohmann::json& j, NumberInfo& v) {
    entity_info_from_json(j, v);
    v.min_value = j.value("min_value", 0.0F);
    v.max_value = j.value("max_value", 0.0F);
    v.step = j.value("step", 0.0F);
    v.unit_of_measurement = j.value("unit_of_measurement", std::string{});
    v.mode = j.value("mode", NumberMode::Auto);
    v.device_class = j.value("device_class", std::string{});
}

inline void to_json(nlohmann::json& j, const SelectInfo& v) {
    entity_info_to_json(j, v);
    j["options"] = v.options;
}
inline void from_json(const nlohmann::json& j, SelectInfo& v) {
    entity_info_from_json(j, v);
    v.options = j.value("options", std::vector<std::string>{});
}

inline void to_json(nlohmann::json& j, const TextInfo& v) {
    entity_info_to_json(j, v);
    j["min_length"] = v.min_length;
    j["max_length"] = v.max_length;
    j["pattern"] = v.pattern;
    j["mode"] = v.mode;
}
inline void from_json(const nlohmann::json& j, TextInfo& v) {
    entity_info_from_json(j, v);
    v.min_length = j.value("min_length", 0U);
    v.max_length = j.value("max_length", 0U);
    v.pattern = j.value("pattern", std::string{});
    v.mode = j.value("mode", TextMode::Text);
}

inline void to_json(nlohmann::json& j, const ButtonInfo& v) {
    entity_info_to_json(j, v);
    j["device_class"] = v.device_class;
}
inline void from_json(const nlohmann::json& j, ButtonInfo& v) {
    entity_info_from_json(j, v);
    v.device_class = j.value("device_class", std::string{});
}

inline void to_json(nlohmann::json& j, const LockInfo& v) {
    entity_info_to_json(j, v);
    j["assumed_state"] = v.assumed_state;
    j["supports_open"] = v.supports_open;
    j["requires_code"] = v.requires_code;
    j["code_format"] = v.code_format;
}
inline void from_json(const nlohmann::json& j, LockInfo& v) {
    entity_info_from_json(j, v);
    v.assumed_state = j.value("assumed_state", false);
    v.supports_open = j.value("supports_open", false);
    v.requires_code = j.value("requires_code", false);
    v.code_format = j.value("code_format", std::string{});
}

inline void to_json(nlohmann::json& j, const MediaPlayerSupportedFormat& v) {
    j = nlohmann::json{{"format", v.format},
                       {"sample_rate", v.sample_rate},
                       {"num_channels", v.num_channels},
                       {"purpose", v.purpose},
                       {"sample_bytes", v.sample_bytes}};
}
inline void from_json(const nlohmann::json& j, MediaPlayerSupportedFormat& v) {
    v.format = j.value("format", std::string{});
    v.sample_rate = j.value("sample_rate", 0U);
    v.num_channels = j.value("num_channels", 0U);
    v.purpose = j.value("purpose", MediaPlayerFormatPurpose::Default);
    v.sample_bytes = j.value("sample_bytes", 0U);
}

inline void to_json(nlohmann::json& j, const MediaPlayerInfo& v) {
    entity_info_to_json(j, v);
    j["supports_pause"] = v.supports_pause;
    j["supported_formats"] = v.supported_formats;
}
inline void from_json(const nlohmann::json& j, MediaPlayerInfo& v) {
    entity_info_from_json(j, v);
    v.supports_pause = j.value("supports_pause", false);
    v.supported_formats = j.value("supported_formats", std::vector<MediaPlayerSupportedFormat>{});
}

inline void to_json(nlohmann::json& j, const CameraInfo& v) {
    entity_info_to_json(j, v);
}
inline void from_json(const nlohmann::json& j, CameraInfo& v) {
    entity_info_from_json(j, v);
}

inline void to_json(nlohmann::json& j, const SirenInfo& v) {
    entity_info_to_json(j, v);
    j["tones"] = v.tones;
    j["supports_duration"] = v.supports_duration;
    j["supports_volume"] = v.supports_volume;
}
inline void from_json(const nlohmann::json& j, SirenInfo& v) {
    entity_info_from_json(j, v);
    v.tones = j.value("tones", std::vector<std::string>{});
    v.supports_duration = j.value("supports_duration", false);
    v.supports_volume = j.value("supports_volume", false);
}

inline void to_json(nlohmann::json& j, const AlarmControlPanelInfo& v) {
    entity_info_to_json(j, v);
    j["supported_features"] = v.supported_features;
    j["requires_code"] = v.requires_code;
    j["requires_code_to_arm"] = v.requires_code_to_arm;
}
inline void from_json(const nlohmann::json& j, AlarmControlPanelInfo& v) {
    entity_info_from_json(j, v);
    v.supported_features = j.value("supported_features", 0U);
    v.requires_code = j.value("requires_code", false);
    v.requires_code_to_arm = j.value("requires_code_to_arm", false);
}

inline void to_json(nlohmann::json& j, const DateInfo& v) {
    entity_info_to_json(j, v);
}
inline void from_json(const nlohmann::json& j, DateInfo& v) {
    entity_info_from_json(j, v);
}

inline void to_json(nlohmann::json& j, const DateTimeInfo& v) {
    entity_info_to_json(j, v);
}
inline void from_json(const nlohmann::json& j, DateTimeInfo& v) {
    entity_info_from_json(j, v);
}

inline void to_json(nlohmann::json& j, const TimeInfo& v) {
    entity_info_to_json(j, v);
}
inline void from_json(const nlohmann::json& j, TimeInfo& v) {
    entity_info_from_json(j, v);
}

inline void to_json(nlohmann::json& j, const ValveInfo& v) {
    entity_info_to_json(j, v);
    j["device_class"] = v.device_class;
    j["assumed_state"] = v.assumed_state;
    j["supports_position"] = v.supports_position;
    j["supports_stop"] = v.supports_stop;
}
inline void from_json(const nlohmann::json& j, ValveInfo& v) {
    entity_info_from_json(j, v);
    v.device_class = j.value("device_class", std::string{});
    v.assumed_state = j.value("assumed_state", false);
    v.supports_position = j.value("supports_position", false);
    v.supports_stop = j.value("supports_stop", false);
}

inline void to_json(nlohmann::json& j, const EventInfo& v) {
    entity_info_to_json(j, v);
    j["device_class"] = v.device_class;
    j["event_types"] = v.event_types;
}
inline void from_json(const nlohmann::json& j, EventInfo& v) {
    entity_info_from_json(j, v);
    v.device_class = j.value("device_class", std::string{});
    v.event_types = j.value("event_types", std::vector<std::string>{});
}

inline void to_json(nlohmann::json& j, const UpdateInfo& v) {
    entity_info_to_json(j, v);
    j["device_class"] = v.device_class;
}
inline void from_json(const nlohmann::json& j, UpdateInfo& v) {
    entity_info_from_json(j, v);
    v.device_class = j.value("device_class", std::string{});
}

inline void to_json(nlohmann::json& j, const WaterHeaterInfo& v) {
    entity_info_to_json(j, v);
    j["min_temperature"] = v.min_temperature;
    j["max_temperature"] = v.max_temperature;
    j["target_temperature_step"] = v.target_temperature_step;
    j["supported_modes"] = v.supported_modes;
    j["supported_features"] = v.supported_features;
    j["temperature_unit"] = v.temperature_unit;
}
inline void from_json(const nlohmann::json& j, WaterHeaterInfo& v) {
    entity_info_from_json(j, v);
    v.min_temperature = j.value("min_temperature", 0.0F);
    v.max_temperature = j.value("max_temperature", 0.0F);
    v.target_temperature_step = j.value("target_temperature_step", 0.0F);
    v.supported_modes = j.value("supported_modes", std::vector<WaterHeaterMode>{});
    v.supported_features = j.value("supported_features", 0U);
    v.temperature_unit = j.value("temperature_unit", TemperatureUnit::Celsius);
}

inline void to_json(nlohmann::json& j, const InfraredInfo& v) {
    entity_info_to_json(j, v);
    j["capabilities"] = v.capabilities;
    j["receiver_frequency"] = v.receiver_frequency;
}
inline void from_json(const nlohmann::json& j, InfraredInfo& v) {
    entity_info_from_json(j, v);
    v.capabilities = j.value("capabilities", 0U);
    v.receiver_frequency = j.value("receiver_frequency", 0U);
}

inline void to_json(nlohmann::json& j, const RadioFrequencyInfo& v) {
    entity_info_to_json(j, v);
    j["capabilities"] = v.capabilities;
    j["frequency_min"] = v.frequency_min;
    j["frequency_max"] = v.frequency_max;
    j["supported_modulations"] = v.supported_modulations;
}
inline void from_json(const nlohmann::json& j, RadioFrequencyInfo& v) {
    entity_info_from_json(j, v);
    v.capabilities = j.value("capabilities", 0U);
    v.frequency_min = j.value("frequency_min", 0U);
    v.frequency_max = j.value("frequency_max", 0U);
    v.supported_modulations = j.value("supported_modulations", 0U);
}

// ---------------------------------------------------------------------------
// Entity state structs (no std::optional fields → macro-friendly).
// ---------------------------------------------------------------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BinarySensorState, key, state, missing_state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SensorState, key, state, missing_state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TextSensorState, key, state, missing_state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SwitchState, key, state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LightState,
                                                key,
                                                state,
                                                brightness,
                                                color_mode,
                                                color_brightness,
                                                red,
                                                green,
                                                blue,
                                                white,
                                                color_temperature,
                                                cold_white,
                                                warm_white,
                                                effect)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CoverState, key, position, tilt, current_operation)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    FanState, key, state, oscillating, direction, speed_level, preset_mode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ClimateState,
                                                key,
                                                mode,
                                                current_temperature,
                                                target_temperature,
                                                target_temperature_low,
                                                target_temperature_high,
                                                action,
                                                fan_mode,
                                                swing_mode,
                                                custom_fan_mode,
                                                preset,
                                                custom_preset,
                                                current_humidity,
                                                target_humidity)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NumberState, key, state, missing_state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SelectState, key, state, missing_state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TextState, key, state, missing_state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LockEntityState, key, state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MediaPlayerStatus, key, state, volume, muted)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SirenState, key, state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AlarmControlPanelStatus, key, state)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DateState, key, missing_state, year, month, day)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DateTimeState, key, missing_state, epoch_seconds)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TimeState, key, missing_state, hour, minute, second)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ValveState, key, position, current_operation)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EventState, key, event_type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UpdateState,
                                                key,
                                                missing_state,
                                                in_progress,
                                                has_progress,
                                                progress,
                                                current_version,
                                                latest_version,
                                                title,
                                                release_summary,
                                                release_url)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaterHeaterState,
                                                key,
                                                current_temperature,
                                                target_temperature,
                                                mode,
                                                state,
                                                target_temperature_low,
                                                target_temperature_high)

// ---------------------------------------------------------------------------
// Device info + discovery.
// ---------------------------------------------------------------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SerialProxyPortInfo, name, port_type)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DeviceInfo,
                                                name,
                                                friendly_name,
                                                mac_address,
                                                bluetooth_mac_address,
                                                esphome_version,
                                                compilation_time,
                                                model,
                                                manufacturer,
                                                project_name,
                                                project_version,
                                                suggested_area,
                                                has_deep_sleep,
                                                webserver_port,
                                                api_encryption_supported,
                                                bluetooth_proxy_feature_flags,
                                                voice_assistant_feature_flags,
                                                zwave_proxy_feature_flags,
                                                zwave_home_id,
                                                serial_proxies)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DiscoveredDevice,
                                                name,
                                                hostname,
                                                address,
                                                port,
                                                mac,
                                                version,
                                                friendly_name,
                                                platform,
                                                board,
                                                network,
                                                project_name,
                                                project_version,
                                                requires_encryption,
                                                supports_encryption,
                                                properties)

// ---------------------------------------------------------------------------
// Log stream.
// ---------------------------------------------------------------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LogEntry, level, message)

// ---------------------------------------------------------------------------
// Bluetooth proxy.
// ---------------------------------------------------------------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleServiceData, uuid, data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleManufacturerData, uuid, data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleAdvertisement,
                                                address,
                                                name,
                                                rssi,
                                                address_type,
                                                service_data,
                                                service_uuids,
                                                manufacturer_data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    BleRawAdvertisement, address, rssi, address_type, data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleScannerState, state, mode, configured_mode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleConnection, address, connected, mtu, error)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleConnectionsFree, free, limit, allocated)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattRead, address, handle, data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattNotifyData, address, handle, data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattDescriptor, uuid, handle)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    BleGattCharacteristic, uuid, handle, properties, descriptors)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattService, uuid, handle, characteristics)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattServices, address, services)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattError, address, handle, error)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattReadResult, address, handle, ok, data, error)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattWriteResult, address, handle, ok, error)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BleGattServicesResult, address, ok, services, error)

// ---------------------------------------------------------------------------
// Serial proxy.
// ---------------------------------------------------------------------------

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SerialProxyConfig, instance, baudrate, flow_control, parity, stop_bits, data_size)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SerialProxyData, instance, data)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SerialProxyResponse, instance, type, status, error_message)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SerialProxyLineStates, rts, dtr)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SerialProxyModemPins, instance, lines)

}  // namespace esphome::api

#endif  // ESPHOME_API_JSON_AVAILABLE
