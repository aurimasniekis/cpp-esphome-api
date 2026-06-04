#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/voice_assistant.hpp>

#include "api.pb.h"

#include <cstdint>
#include <string>
#include <utility>

namespace esphome::api {

void VoiceAssistant::subscribe(VoiceAssistantSubscribeFlag flags,
                               RequestHandler handler,
                               const bool subscribe) {
    request_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::VoiceAssistantRequest);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::VoiceAssistantRequest&>(msg);
        if (request_handler_) {
            VoiceRequest req;
            req.start = resp.start();
            req.conversation_id = resp.conversation_id();
            req.flags = static_cast<VoiceAssistantRequestFlag>(resp.flags());
            req.audio_settings.noise_suppression_level =
                resp.audio_settings().noise_suppression_level();
            req.audio_settings.auto_gain = resp.audio_settings().auto_gain();
            req.audio_settings.volume_multiplier = resp.audio_settings().volume_multiplier();
            req.wake_word_phrase = resp.wake_word_phrase();
            request_handler_(req);
        }
    });

    proto::SubscribeVoiceAssistantRequest req;
    req.set_subscribe(subscribe);
    req.set_flags(static_cast<std::uint32_t>(flags));
    client_.send(req);
}

void VoiceAssistant::on_event(EventHandler handler) {
    event_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::VoiceAssistantEventResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::VoiceAssistantEventResponse&>(msg);
        if (event_handler_) {
            VoiceEvent event;
            event.event_type = static_cast<VoiceAssistantEvent>(resp.event_type());
            for (int i = 0; i < resp.data_size(); ++i) {
                event.data.emplace_back(resp.data(i).name(), resp.data(i).value());
            }
            event_handler_(event);
        }
    });
}

void VoiceAssistant::send_response(const std::uint32_t port, const bool error) const {
    proto::VoiceAssistantResponse resp;
    resp.set_port(port);
    resp.set_error(error);
    client_.send(resp);
}

void VoiceAssistant::on_audio(AudioHandler handler) {
    audio_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::VoiceAssistantAudio);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::VoiceAssistantAudio&>(msg);
        if (audio_handler_) {
            VoiceAudio audio;
            const std::string& bytes = resp.data();
            audio.data.assign(bytes.begin(), bytes.end());
            audio.end = resp.end();
            audio_handler_(audio);
        }
    });
}

void VoiceAssistant::send_audio(const ByteView data, const bool end) const {
    proto::VoiceAssistantAudio audio;
    audio.set_data(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
    audio.set_end(end);
    client_.send(audio);
}

void VoiceAssistant::announce(const std::string& media_id,
                              const std::string& text,
                              const std::string& preannounce_media_id,
                              const bool start_conversation) const {
    proto::VoiceAssistantAnnounceRequest req;
    req.set_media_id(media_id);
    req.set_text(text);
    req.set_preannounce_media_id(preannounce_media_id);
    req.set_start_conversation(start_conversation);
    client_.send(req);
}

void VoiceAssistant::on_announce_finished(AnnounceFinishedHandler handler) {
    announce_finished_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::VoiceAssistantAnnounceFinished);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::VoiceAssistantAnnounceFinished&>(msg);
        if (announce_finished_handler_) {
            announce_finished_handler_(resp.success());
        }
    });
}

void VoiceAssistant::request_configuration(ConfigurationHandler handler) {
    configuration_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::VoiceAssistantConfigurationResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::VoiceAssistantConfigurationResponse&>(msg);
        if (configuration_handler_) {
            VoiceConfiguration config;
            for (int i = 0; i < resp.available_wake_words_size(); ++i) {
                const auto& src = resp.available_wake_words(i);
                VoiceWakeWord word;
                word.id = src.id();
                word.wake_word = src.wake_word();
                for (int j = 0; j < src.trained_languages_size(); ++j) {
                    word.trained_languages.push_back(src.trained_languages(j));
                }
                config.available_wake_words.push_back(std::move(word));
            }
            for (int i = 0; i < resp.active_wake_words_size(); ++i) {
                config.active_wake_words.push_back(resp.active_wake_words(i));
            }
            config.max_active_wake_words = resp.max_active_wake_words();
            configuration_handler_(config);
        }
    });

    const proto::VoiceAssistantConfigurationRequest req;
    client_.send(req);
}

void VoiceAssistant::set_configuration(const std::vector<std::string>& active_wake_words) const {
    proto::VoiceAssistantSetConfiguration req;
    for (const std::string& word : active_wake_words) {
        req.add_active_wake_words(word);
    }
    client_.send(req);
}

void VoiceAssistant::on_timer_event(TimerEventHandler handler) {
    timer_event_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::VoiceAssistantTimerEventResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::VoiceAssistantTimerEventResponse&>(msg);
        if (timer_event_handler_) {
            VoiceTimerEvent event;
            event.event_type = static_cast<VoiceAssistantTimerEvent>(resp.event_type());
            event.timer_id = resp.timer_id();
            event.name = resp.name();
            event.total_seconds = resp.total_seconds();
            event.seconds_left = resp.seconds_left();
            event.is_active = resp.is_active();
            timer_event_handler_(event);
        }
    });
}

}  // namespace esphome::api
