#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/log_stream.hpp>

#include "api.pb.h"

#include <utility>

namespace esphome::api {

void LogStream::subscribe(LogLevel level, Handler handler, const bool dump_config) {
    handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::SubscribeLogsResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::SubscribeLogsResponse&>(msg);
        if (handler_) {
            LogEntry entry;
            entry.level = static_cast<LogLevel>(resp.level());
            entry.message = resp.message();
            handler_(entry);
        }
    });

    proto::SubscribeLogsRequest req;
    req.set_level(static_cast<proto::LogLevel>(level));
    req.set_dump_config(dump_config);
    client_.send(req);
}

}  // namespace esphome::api
