#pragma once

/// @file
/// @brief Voice Assistant subsystem.

#include <esphome/api/bytes.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/subsystem.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace esphome::api {

/// Audio capture/processing settings the device requests for a pipeline run
/// (api: `VoiceAssistantRequest.audio_settings`).
struct VoiceAudioSettings {
    std::uint32_t noise_suppression_level = 0;
    std::uint32_t auto_gain = 0;
    float volume_multiplier = 0.0F;
};

/// The device asking the client to start or stop a voice pipeline
/// (api: `VoiceAssistantRequest`).
struct VoiceRequest {
    bool start = false;
    std::string conversation_id;
    VoiceAssistantRequestFlag flags = VoiceAssistantRequestFlag::None;
    VoiceAudioSettings audio_settings;
    std::string wake_word_phrase;
};

/// A pipeline event emitted by the device
/// (api: `VoiceAssistantEventResponse`). `data` mirrors the repeated
/// name/value pairs of `VoiceAssistantEventData`.
struct VoiceEvent {
    VoiceAssistantEvent event_type = VoiceAssistantEvent::Error;
    std::vector<std::pair<std::string, std::string>> data;
};

/// A chunk of audio streamed over the API (api: `VoiceAssistantAudio`).
struct VoiceAudio {
    std::vector<std::uint8_t> data;
    bool end = false;
};

/// A timer event surfaced by the device (api: `VoiceAssistantTimerEventResponse`).
struct VoiceTimerEvent {
    VoiceAssistantTimerEvent event_type = VoiceAssistantTimerEvent::Started;
    std::string timer_id;
    std::string name;
    std::uint32_t total_seconds = 0;
    std::uint32_t seconds_left = 0;
    bool is_active = false;
};

/// A wake word the device can be configured to listen for
/// (api: `VoiceAssistantWakeWord`).
struct VoiceWakeWord {
    std::string id;
    std::string wake_word;
    std::vector<std::string> trained_languages;
};

/// The device's voice-assistant configuration
/// (api: `VoiceAssistantConfigurationResponse`).
struct VoiceConfiguration {
    std::vector<VoiceWakeWord> available_wake_words;
    std::vector<std::string> active_wake_words;
    std::uint32_t max_active_wake_words = 0;
};

/// Drives the device's voice-assistant pipeline: subscribing to start/stop
/// requests, relaying events and audio, announcing media, and managing the
/// wake-word configuration.
class VoiceAssistant : public Subsystem {
public:
    using RequestHandler = std::function<void(const VoiceRequest&)>;
    using EventHandler = std::function<void(const VoiceEvent&)>;
    using AudioHandler = std::function<void(const VoiceAudio&)>;
    using AnnounceFinishedHandler = std::function<void(bool /*success*/)>;
    using ConfigurationHandler = std::function<void(const VoiceConfiguration&)>;
    using TimerEventHandler = std::function<void(const VoiceTimerEvent&)>;

    explicit VoiceAssistant(Client& client) : Subsystem(client) {}

    /// Subscribe to (or unsubscribe from) the device's voice-assistant
    /// requests. `flags` selects optional features such as API audio.
    /// `handler` receives each VoiceRequest the device sends.
    void
    subscribe(VoiceAssistantSubscribeFlag flags, RequestHandler handler, bool subscribe = true);

    /// Register a callback for each pipeline event (VoiceAssistantEventResponse).
    void on_event(EventHandler handler);

    /// Reply to a start request, telling the device which UDP `port` to stream
    /// audio to (0 when streaming over the API), or that an `error` occurred.
    void send_response(std::uint32_t port, bool error = false) const;

    /// Register a callback for inbound audio chunks (VoiceAssistantAudio).
    void on_audio(AudioHandler handler);

    /// Stream an audio chunk to the device. Set `end` on the final chunk.
    void send_audio(ByteView data, bool end = false) const;

    /// Ask the device to announce media and/or text. `start_conversation`
    /// begins a pipeline run once the announcement finishes.
    void announce(const std::string& media_id,
                  const std::string& text = {},
                  const std::string& preannounce_media_id = {},
                  bool start_conversation = false) const;

    /// Register a callback fired when an announcement finishes.
    void on_announce_finished(AnnounceFinishedHandler handler);

    /// Request the device's voice-assistant configuration. `handler` receives
    /// the VoiceAssistantConfigurationResponse.
    void request_configuration(ConfigurationHandler handler);

    /// Set the device's active wake words by id.
    void set_configuration(const std::vector<std::string>& active_wake_words) const;

    /// Register a callback for timer events (VoiceAssistantTimerEventResponse).
    void on_timer_event(TimerEventHandler handler);

private:
    RequestHandler request_handler_;
    EventHandler event_handler_;
    AudioHandler audio_handler_;
    AnnounceFinishedHandler announce_finished_handler_;
    ConfigurationHandler configuration_handler_;
    TimerEventHandler timer_event_handler_;
};

}  // namespace esphome::api
