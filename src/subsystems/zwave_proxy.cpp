#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/zwave_proxy.hpp>

#include "api.pb.h"

#include <utility>

namespace esphome::api {

void ZWaveProxy::on_frame(FrameHandler handler) {
    frame_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::ZWaveProxyFrame);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& frame = static_cast<const proto::ZWaveProxyFrame&>(msg);
        if (frame_handler_) {
            ZWaveProxyFrame out;
            out.data = frame.data();
            frame_handler_(out);
        }
    });
}

void ZWaveProxy::send_frame(const std::string& data) const {
    proto::ZWaveProxyFrame frame;
    frame.set_data(data);
    client_.send(frame);
}

void ZWaveProxy::request(ZWaveProxyRequestType type, const std::string& data) const {
    proto::ZWaveProxyRequest req;
    req.set_type(static_cast<proto::ZWaveProxyRequestType>(type));
    req.set_data(data);
    client_.send(req);
}

}  // namespace esphome::api
