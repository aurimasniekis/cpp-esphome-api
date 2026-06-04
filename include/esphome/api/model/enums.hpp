#pragma once

/// @file
/// @brief Typed mirrors of the ESPHome protobuf enums. Values match the proto
///        exactly, so conversion to/from the generated enums is a static_cast.
///        Generated from proto/api.proto — regenerate if the protos are re-vendored.

#include <cstdint>

namespace esphome::api {

/// Mirror of proto enum SerialProxyPortType.
enum class SerialProxyPortType : std::int32_t {
    Ttl = 0,    // SERIAL_PROXY_PORT_TYPE_TTL
    Rs232 = 1,  // SERIAL_PROXY_PORT_TYPE_RS232
    Rs485 = 2,  // SERIAL_PROXY_PORT_TYPE_RS485
};

/// Mirror of proto enum EntityCategory.
enum class EntityCategory : std::int32_t {
    None = 0,        // ENTITY_CATEGORY_NONE
    Config = 1,      // ENTITY_CATEGORY_CONFIG
    Diagnostic = 2,  // ENTITY_CATEGORY_DIAGNOSTIC
};

/// Mirror of proto enum LegacyCoverState.
enum class LegacyCoverState : std::int32_t {
    Open = 0,    // LEGACY_COVER_STATE_OPEN
    Closed = 1,  // LEGACY_COVER_STATE_CLOSED
};

/// Mirror of proto enum CoverOperation.
enum class CoverOperation : std::int32_t {
    Idle = 0,       // COVER_OPERATION_IDLE
    IsOpening = 1,  // COVER_OPERATION_IS_OPENING
    IsClosing = 2,  // COVER_OPERATION_IS_CLOSING
};

/// Mirror of proto enum LegacyCoverCommand.
enum class LegacyCoverCommand : std::int32_t {
    Open = 0,   // LEGACY_COVER_COMMAND_OPEN
    Close = 1,  // LEGACY_COVER_COMMAND_CLOSE
    Stop = 2,   // LEGACY_COVER_COMMAND_STOP
};

/// Mirror of proto enum FanSpeed.
enum class FanSpeed : std::int32_t {
    Low = 0,     // FAN_SPEED_LOW
    Medium = 1,  // FAN_SPEED_MEDIUM
    High = 2,    // FAN_SPEED_HIGH
};

/// Mirror of proto enum FanDirection.
enum class FanDirection : std::int32_t {
    Forward = 0,  // FAN_DIRECTION_FORWARD
    Reverse = 1,  // FAN_DIRECTION_REVERSE
};

/// Mirror of proto enum ColorMode.
enum class ColorMode : std::int32_t {
    Unknown = 0,               // COLOR_MODE_UNKNOWN
    OnOff = 1,                 // COLOR_MODE_ON_OFF
    LegacyBrightness = 2,      // COLOR_MODE_LEGACY_BRIGHTNESS
    Brightness = 3,            // COLOR_MODE_BRIGHTNESS
    White = 7,                 // COLOR_MODE_WHITE
    ColorTemperature = 11,     // COLOR_MODE_COLOR_TEMPERATURE
    ColdWarmWhite = 19,        // COLOR_MODE_COLD_WARM_WHITE
    Rgb = 35,                  // COLOR_MODE_RGB
    RgbWhite = 39,             // COLOR_MODE_RGB_WHITE
    RgbColorTemperature = 47,  // COLOR_MODE_RGB_COLOR_TEMPERATURE
    RgbColdWarmWhite = 51,     // COLOR_MODE_RGB_COLD_WARM_WHITE
};

/// Mirror of proto enum SensorStateClass.
enum class SensorStateClass : std::int32_t {
    None = 0,              // STATE_CLASS_NONE
    Measurement = 1,       // STATE_CLASS_MEASUREMENT
    TotalIncreasing = 2,   // STATE_CLASS_TOTAL_INCREASING
    Total = 3,             // STATE_CLASS_TOTAL
    MeasurementAngle = 4,  // STATE_CLASS_MEASUREMENT_ANGLE
};

/// Mirror of proto enum SensorLastResetType.
enum class SensorLastResetType : std::int32_t {
    None = 0,   // LAST_RESET_NONE
    Never = 1,  // LAST_RESET_NEVER
    Auto = 2,   // LAST_RESET_AUTO
};

/// Mirror of proto enum LogLevel.
enum class LogLevel : std::int32_t {
    None = 0,         // LOG_LEVEL_NONE
    Error = 1,        // LOG_LEVEL_ERROR
    Warn = 2,         // LOG_LEVEL_WARN
    Info = 3,         // LOG_LEVEL_INFO
    Config = 4,       // LOG_LEVEL_CONFIG
    Debug = 5,        // LOG_LEVEL_DEBUG
    Verbose = 6,      // LOG_LEVEL_VERBOSE
    VeryVerbose = 7,  // LOG_LEVEL_VERY_VERBOSE
};

/// Mirror of proto enum DSTRuleType.
enum class DSTRuleType : std::int32_t {
    None = 0,          // DST_RULE_TYPE_NONE
    MonthWeekDay = 1,  // DST_RULE_TYPE_MONTH_WEEK_DAY
    JulianNoLeap = 2,  // DST_RULE_TYPE_JULIAN_NO_LEAP
    DayOfYear = 3,     // DST_RULE_TYPE_DAY_OF_YEAR
};

/// Mirror of proto enum ServiceArgType.
enum class ServiceArgType : std::int32_t {
    Bool = 0,         // SERVICE_ARG_TYPE_BOOL
    Int = 1,          // SERVICE_ARG_TYPE_INT
    Float = 2,        // SERVICE_ARG_TYPE_FLOAT
    String = 3,       // SERVICE_ARG_TYPE_STRING
    BoolArray = 4,    // SERVICE_ARG_TYPE_BOOL_ARRAY
    IntArray = 5,     // SERVICE_ARG_TYPE_INT_ARRAY
    FloatArray = 6,   // SERVICE_ARG_TYPE_FLOAT_ARRAY
    StringArray = 7,  // SERVICE_ARG_TYPE_STRING_ARRAY
};

/// Mirror of proto enum SupportsResponseType.
enum class SupportsResponseType : std::int32_t {
    None = 0,      // SUPPORTS_RESPONSE_NONE
    Optional = 1,  // SUPPORTS_RESPONSE_OPTIONAL
    Only = 2,      // SUPPORTS_RESPONSE_ONLY
    Status = 100,  // SUPPORTS_RESPONSE_STATUS
};

/// Mirror of proto enum TemperatureUnit.
enum class TemperatureUnit : std::int32_t {
    Celsius = 0,     // TEMPERATURE_UNIT_CELSIUS
    Fahrenheit = 1,  // TEMPERATURE_UNIT_FAHRENHEIT
    Kelvin = 2,      // TEMPERATURE_UNIT_KELVIN
};

/// Mirror of proto enum ClimateMode.
enum class ClimateMode : std::int32_t {
    Off = 0,       // CLIMATE_MODE_OFF
    HeatCool = 1,  // CLIMATE_MODE_HEAT_COOL
    Cool = 2,      // CLIMATE_MODE_COOL
    Heat = 3,      // CLIMATE_MODE_HEAT
    FanOnly = 4,   // CLIMATE_MODE_FAN_ONLY
    Dry = 5,       // CLIMATE_MODE_DRY
    Auto = 6,      // CLIMATE_MODE_AUTO
};

/// Mirror of proto enum ClimateFanMode.
enum class ClimateFanMode : std::int32_t {
    On = 0,       // CLIMATE_FAN_ON
    Off = 1,      // CLIMATE_FAN_OFF
    Auto = 2,     // CLIMATE_FAN_AUTO
    Low = 3,      // CLIMATE_FAN_LOW
    Medium = 4,   // CLIMATE_FAN_MEDIUM
    High = 5,     // CLIMATE_FAN_HIGH
    Middle = 6,   // CLIMATE_FAN_MIDDLE
    Focus = 7,    // CLIMATE_FAN_FOCUS
    Diffuse = 8,  // CLIMATE_FAN_DIFFUSE
    Quiet = 9,    // CLIMATE_FAN_QUIET
};

/// Mirror of proto enum ClimateSwingMode.
enum class ClimateSwingMode : std::int32_t {
    Off = 0,         // CLIMATE_SWING_OFF
    Both = 1,        // CLIMATE_SWING_BOTH
    Vertical = 2,    // CLIMATE_SWING_VERTICAL
    Horizontal = 3,  // CLIMATE_SWING_HORIZONTAL
};

/// Mirror of proto enum ClimateAction.
enum class ClimateAction : std::int32_t {
    Off = 0,         // CLIMATE_ACTION_OFF
    Cooling = 2,     // CLIMATE_ACTION_COOLING
    Heating = 3,     // CLIMATE_ACTION_HEATING
    Idle = 4,        // CLIMATE_ACTION_IDLE
    Drying = 5,      // CLIMATE_ACTION_DRYING
    Fan = 6,         // CLIMATE_ACTION_FAN
    Defrosting = 7,  // CLIMATE_ACTION_DEFROSTING
};

/// Mirror of proto enum ClimatePreset.
enum class ClimatePreset : std::int32_t {
    None = 0,      // CLIMATE_PRESET_NONE
    Home = 1,      // CLIMATE_PRESET_HOME
    Away = 2,      // CLIMATE_PRESET_AWAY
    Boost = 3,     // CLIMATE_PRESET_BOOST
    Comfort = 4,   // CLIMATE_PRESET_COMFORT
    Eco = 5,       // CLIMATE_PRESET_ECO
    Sleep = 6,     // CLIMATE_PRESET_SLEEP
    Activity = 7,  // CLIMATE_PRESET_ACTIVITY
};

/// Mirror of proto enum WaterHeaterMode.
enum class WaterHeaterMode : std::int32_t {
    Off = 0,          // WATER_HEATER_MODE_OFF
    Eco = 1,          // WATER_HEATER_MODE_ECO
    Electric = 2,     // WATER_HEATER_MODE_ELECTRIC
    Performance = 3,  // WATER_HEATER_MODE_PERFORMANCE
    HighDemand = 4,   // WATER_HEATER_MODE_HIGH_DEMAND
    HeatPump = 5,     // WATER_HEATER_MODE_HEAT_PUMP
    Gas = 6,          // WATER_HEATER_MODE_GAS
};

/// Mirror of proto enum WaterHeaterCommandHasField.
enum class WaterHeaterCommandHasField : std::int32_t {
    None = 0,                    // WATER_HEATER_COMMAND_HAS_NONE
    Mode = 1,                    // WATER_HEATER_COMMAND_HAS_MODE
    TargetTemperature = 2,       // WATER_HEATER_COMMAND_HAS_TARGET_TEMPERATURE
    State = 4,                   // WATER_HEATER_COMMAND_HAS_STATE
    TargetTemperatureLow = 8,    // WATER_HEATER_COMMAND_HAS_TARGET_TEMPERATURE_LOW
    TargetTemperatureHigh = 16,  // WATER_HEATER_COMMAND_HAS_TARGET_TEMPERATURE_HIGH
    OnState = 32,                // WATER_HEATER_COMMAND_HAS_ON_STATE
    AwayState = 64,              // WATER_HEATER_COMMAND_HAS_AWAY_STATE
};

/// Mirror of proto enum NumberMode.
enum class NumberMode : std::int32_t {
    Auto = 0,    // NUMBER_MODE_AUTO
    Box = 1,     // NUMBER_MODE_BOX
    Slider = 2,  // NUMBER_MODE_SLIDER
};

/// Mirror of proto enum LockState.
enum class LockState : std::int32_t {
    None = 0,       // LOCK_STATE_NONE
    Locked = 1,     // LOCK_STATE_LOCKED
    Unlocked = 2,   // LOCK_STATE_UNLOCKED
    Jammed = 3,     // LOCK_STATE_JAMMED
    Locking = 4,    // LOCK_STATE_LOCKING
    Unlocking = 5,  // LOCK_STATE_UNLOCKING
    Opening = 6,    // LOCK_STATE_OPENING
    Open = 7,       // LOCK_STATE_OPEN
};

/// Mirror of proto enum MediaPlayerState.
enum class MediaPlayerState : std::int32_t {
    None = 0,        // MEDIA_PLAYER_STATE_NONE
    Idle = 1,        // MEDIA_PLAYER_STATE_IDLE
    Playing = 2,     // MEDIA_PLAYER_STATE_PLAYING
    Paused = 3,      // MEDIA_PLAYER_STATE_PAUSED
    Announcing = 4,  // MEDIA_PLAYER_STATE_ANNOUNCING
    Off = 5,         // MEDIA_PLAYER_STATE_OFF
    On = 6,          // MEDIA_PLAYER_STATE_ON
};

/// Mirror of proto enum MediaPlayerCommand.
enum class MediaPlayerCommand : std::int32_t {
    Play = 0,            // MEDIA_PLAYER_COMMAND_PLAY
    Pause = 1,           // MEDIA_PLAYER_COMMAND_PAUSE
    Stop = 2,            // MEDIA_PLAYER_COMMAND_STOP
    Mute = 3,            // MEDIA_PLAYER_COMMAND_MUTE
    Unmute = 4,          // MEDIA_PLAYER_COMMAND_UNMUTE
    Toggle = 5,          // MEDIA_PLAYER_COMMAND_TOGGLE
    VolumeUp = 6,        // MEDIA_PLAYER_COMMAND_VOLUME_UP
    VolumeDown = 7,      // MEDIA_PLAYER_COMMAND_VOLUME_DOWN
    Enqueue = 8,         // MEDIA_PLAYER_COMMAND_ENQUEUE
    RepeatOne = 9,       // MEDIA_PLAYER_COMMAND_REPEAT_ONE
    RepeatOff = 10,      // MEDIA_PLAYER_COMMAND_REPEAT_OFF
    ClearPlaylist = 11,  // MEDIA_PLAYER_COMMAND_CLEAR_PLAYLIST
    TurnOn = 12,         // MEDIA_PLAYER_COMMAND_TURN_ON
    TurnOff = 13,        // MEDIA_PLAYER_COMMAND_TURN_OFF
};

/// Mirror of proto enum MediaPlayerFormatPurpose.
enum class MediaPlayerFormatPurpose : std::int32_t {
    Default = 0,       // MEDIA_PLAYER_FORMAT_PURPOSE_DEFAULT
    Announcement = 1,  // MEDIA_PLAYER_FORMAT_PURPOSE_ANNOUNCEMENT
};

/// Mirror of proto enum BluetoothDeviceRequestType.
enum class BluetoothDeviceRequestType : std::int32_t {
    Connect = 0,                // BLUETOOTH_DEVICE_REQUEST_TYPE_CONNECT
    Disconnect = 1,             // BLUETOOTH_DEVICE_REQUEST_TYPE_DISCONNECT
    Pair = 2,                   // BLUETOOTH_DEVICE_REQUEST_TYPE_PAIR
    Unpair = 3,                 // BLUETOOTH_DEVICE_REQUEST_TYPE_UNPAIR
    ConnectV3WithCache = 4,     // BLUETOOTH_DEVICE_REQUEST_TYPE_CONNECT_V3_WITH_CACHE
    ConnectV3WithoutCache = 5,  // BLUETOOTH_DEVICE_REQUEST_TYPE_CONNECT_V3_WITHOUT_CACHE
    ClearCache = 6,             // BLUETOOTH_DEVICE_REQUEST_TYPE_CLEAR_CACHE
};

/// Mirror of proto enum BluetoothScannerState.
enum class BluetoothScannerState : std::int32_t {
    Idle = 0,      // BLUETOOTH_SCANNER_STATE_IDLE
    Starting = 1,  // BLUETOOTH_SCANNER_STATE_STARTING
    Running = 2,   // BLUETOOTH_SCANNER_STATE_RUNNING
    Failed = 3,    // BLUETOOTH_SCANNER_STATE_FAILED
    Stopping = 4,  // BLUETOOTH_SCANNER_STATE_STOPPING
    Stopped = 5,   // BLUETOOTH_SCANNER_STATE_STOPPED
};

/// Mirror of proto enum BluetoothScannerMode.
enum class BluetoothScannerMode : std::int32_t {
    Passive = 0,  // BLUETOOTH_SCANNER_MODE_PASSIVE
    Active = 1,   // BLUETOOTH_SCANNER_MODE_ACTIVE
};

/// Mirror of proto enum VoiceAssistantSubscribeFlag.
enum class VoiceAssistantSubscribeFlag : std::int32_t {
    None = 0,      // VOICE_ASSISTANT_SUBSCRIBE_NONE
    ApiAudio = 1,  // VOICE_ASSISTANT_SUBSCRIBE_API_AUDIO
};

/// Mirror of proto enum VoiceAssistantRequestFlag.
enum class VoiceAssistantRequestFlag : std::int32_t {
    None = 0,         // VOICE_ASSISTANT_REQUEST_NONE
    UseVad = 1,       // VOICE_ASSISTANT_REQUEST_USE_VAD
    UseWakeWord = 2,  // VOICE_ASSISTANT_REQUEST_USE_WAKE_WORD
};

/// Mirror of proto enum VoiceAssistantEvent.
enum class VoiceAssistantEvent : std::int32_t {
    Error = 0,             // VOICE_ASSISTANT_ERROR
    RunStart = 1,          // VOICE_ASSISTANT_RUN_START
    RunEnd = 2,            // VOICE_ASSISTANT_RUN_END
    SttStart = 3,          // VOICE_ASSISTANT_STT_START
    SttEnd = 4,            // VOICE_ASSISTANT_STT_END
    IntentStart = 5,       // VOICE_ASSISTANT_INTENT_START
    IntentEnd = 6,         // VOICE_ASSISTANT_INTENT_END
    TtsStart = 7,          // VOICE_ASSISTANT_TTS_START
    TtsEnd = 8,            // VOICE_ASSISTANT_TTS_END
    WakeWordStart = 9,     // VOICE_ASSISTANT_WAKE_WORD_START
    WakeWordEnd = 10,      // VOICE_ASSISTANT_WAKE_WORD_END
    SttVadStart = 11,      // VOICE_ASSISTANT_STT_VAD_START
    SttVadEnd = 12,        // VOICE_ASSISTANT_STT_VAD_END
    TtsStreamStart = 98,   // VOICE_ASSISTANT_TTS_STREAM_START
    TtsStreamEnd = 99,     // VOICE_ASSISTANT_TTS_STREAM_END
    IntentProgress = 100,  // VOICE_ASSISTANT_INTENT_PROGRESS
};

/// Mirror of proto enum VoiceAssistantTimerEvent.
enum class VoiceAssistantTimerEvent : std::int32_t {
    Started = 0,    // VOICE_ASSISTANT_TIMER_STARTED
    Updated = 1,    // VOICE_ASSISTANT_TIMER_UPDATED
    Cancelled = 2,  // VOICE_ASSISTANT_TIMER_CANCELLED
    Finished = 3,   // VOICE_ASSISTANT_TIMER_FINISHED
};

/// Mirror of proto enum AlarmControlPanelState.
enum class AlarmControlPanelState : std::int32_t {
    Disarmed = 0,           // ALARM_STATE_DISARMED
    ArmedHome = 1,          // ALARM_STATE_ARMED_HOME
    ArmedAway = 2,          // ALARM_STATE_ARMED_AWAY
    ArmedNight = 3,         // ALARM_STATE_ARMED_NIGHT
    ArmedVacation = 4,      // ALARM_STATE_ARMED_VACATION
    ArmedCustomBypass = 5,  // ALARM_STATE_ARMED_CUSTOM_BYPASS
    Pending = 6,            // ALARM_STATE_PENDING
    Arming = 7,             // ALARM_STATE_ARMING
    Disarming = 8,          // ALARM_STATE_DISARMING
    Triggered = 9,          // ALARM_STATE_TRIGGERED
};

/// Mirror of proto enum AlarmControlPanelStateCommand.
enum class AlarmControlPanelStateCommand : std::int32_t {
    Disarm = 0,           // ALARM_CONTROL_PANEL_DISARM
    ArmAway = 1,          // ALARM_CONTROL_PANEL_ARM_AWAY
    ArmHome = 2,          // ALARM_CONTROL_PANEL_ARM_HOME
    ArmNight = 3,         // ALARM_CONTROL_PANEL_ARM_NIGHT
    ArmVacation = 4,      // ALARM_CONTROL_PANEL_ARM_VACATION
    ArmCustomBypass = 5,  // ALARM_CONTROL_PANEL_ARM_CUSTOM_BYPASS
    Trigger = 6,          // ALARM_CONTROL_PANEL_TRIGGER
};

/// Mirror of proto enum TextMode.
enum class TextMode : std::int32_t {
    Text = 0,      // TEXT_MODE_TEXT
    Password = 1,  // TEXT_MODE_PASSWORD
};

/// Mirror of proto enum ValveOperation.
enum class ValveOperation : std::int32_t {
    Idle = 0,       // VALVE_OPERATION_IDLE
    IsOpening = 1,  // VALVE_OPERATION_IS_OPENING
    IsClosing = 2,  // VALVE_OPERATION_IS_CLOSING
};

/// Mirror of proto enum UpdateCommand.
enum class UpdateCommand : std::int32_t {
    None = 0,    // UPDATE_COMMAND_NONE
    Update = 1,  // UPDATE_COMMAND_UPDATE
    Check = 2,   // UPDATE_COMMAND_CHECK
};

/// Mirror of proto enum ZWaveProxyRequestType.
enum class ZWaveProxyRequestType : std::int32_t {
    Subscribe = 0,     // ZWAVE_PROXY_REQUEST_TYPE_SUBSCRIBE
    Unsubscribe = 1,   // ZWAVE_PROXY_REQUEST_TYPE_UNSUBSCRIBE
    HomeIdChange = 2,  // ZWAVE_PROXY_REQUEST_TYPE_HOME_ID_CHANGE
};

/// Mirror of proto enum SerialProxyParity.
enum class SerialProxyParity : std::int32_t {
    None = 0,  // SERIAL_PROXY_PARITY_NONE
    Even = 1,  // SERIAL_PROXY_PARITY_EVEN
    Odd = 2,   // SERIAL_PROXY_PARITY_ODD
};

/// Mirror of proto enum SerialProxyRequestType.
enum class SerialProxyRequestType : std::int32_t {
    Subscribe = 0,    // SERIAL_PROXY_REQUEST_TYPE_SUBSCRIBE
    Unsubscribe = 1,  // SERIAL_PROXY_REQUEST_TYPE_UNSUBSCRIBE
    Flush = 2,        // SERIAL_PROXY_REQUEST_TYPE_FLUSH
};

/// Mirror of proto enum SerialProxyStatus.
enum class SerialProxyStatus : std::int32_t {
    Ok = 0,              // SERIAL_PROXY_STATUS_OK
    AssumedSuccess = 1,  // SERIAL_PROXY_STATUS_ASSUMED_SUCCESS
    Error = 2,           // SERIAL_PROXY_STATUS_ERROR
    Timeout = 3,         // SERIAL_PROXY_STATUS_TIMEOUT
    NotSupported = 4,    // SERIAL_PROXY_STATUS_NOT_SUPPORTED
};

}  // namespace esphome::api
