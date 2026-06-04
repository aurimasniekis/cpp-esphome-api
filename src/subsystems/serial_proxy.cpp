#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>

#include "api.pb.h"

#include <utility>

namespace esphome::api {

void SerialProxy::ensure_data_handler() {
    if (data_registered_) {
        return;
    }
    data_registered_ = true;
    constexpr auto id = static_cast<std::uint32_t>(MessageId::SerialProxyDataReceived);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::SerialProxyDataReceived&>(msg);
        SerialProxyData out;
        out.instance = resp.instance();
        out.data = resp.data();
        if (data_handler_) {
            data_handler_(out);
        }
        if (const auto it = instance_data_handlers_.find(out.instance);
            it != instance_data_handlers_.end() && it->second) {
            it->second(out);
        }
    });
}

void SerialProxy::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    ensure_data_handler();
}

void SerialProxy::on_data(const std::uint32_t instance, DataHandler handler) {
    instance_data_handlers_[instance] = std::move(handler);
    ensure_data_handler();
}

void SerialProxy::on_request_response(ResponseHandler handler) {
    response_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::SerialProxyRequestResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::SerialProxyRequestResponse&>(msg);
        if (response_handler_) {
            SerialProxyResponse out;
            out.instance = resp.instance();
            out.type = static_cast<SerialProxyRequestType>(resp.type());
            out.status = static_cast<SerialProxyStatus>(resp.status());
            out.error_message = resp.error_message();
            response_handler_(out);
        }
    });
}

void SerialProxy::ensure_modem_pins_handler() {
    if (modem_pins_registered_) {
        return;
    }
    modem_pins_registered_ = true;
    constexpr auto id = static_cast<std::uint32_t>(MessageId::SerialProxyGetModemPinsResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::SerialProxyGetModemPinsResponse&>(msg);
        SerialProxyModemPins out;
        out.instance = resp.instance();
        out.lines = SerialProxyLineStates::from_bits(resp.line_states());

        // Dispatch any one-shot handlers waiting on this instance first.
        if (const auto it = pending_modem_pins_.find(out.instance);
            it != pending_modem_pins_.end()) {
            const std::vector<ModemPinsHandler> once = std::move(it->second);
            pending_modem_pins_.erase(it);
            for (const auto& cb : once) {
                cb(out);
            }
        }
        if (modem_pins_handler_) {
            modem_pins_handler_(out);
        }
    });
}

void SerialProxy::on_modem_pins(ModemPinsHandler handler) {
    modem_pins_handler_ = std::move(handler);
    ensure_modem_pins_handler();
}

void SerialProxy::request(const std::uint32_t instance, SerialProxyRequestType type) const {
    proto::SerialProxyRequest req;
    req.set_instance(instance);
    req.set_type(static_cast<proto::SerialProxyRequestType>(type));
    client_.send(req);
}

void SerialProxy::configure(const SerialProxyConfig& config) const {
    proto::SerialProxyConfigureRequest req;
    req.set_instance(config.instance);
    req.set_baudrate(config.baudrate);
    req.set_flow_control(config.flow_control);
    req.set_parity(static_cast<proto::SerialProxyParity>(config.parity));
    req.set_stop_bits(config.stop_bits);
    req.set_data_size(config.data_size);
    client_.send(req);
}

void SerialProxy::write(const std::uint32_t instance, const std::string& data) const {
    proto::SerialProxyWriteRequest req;
    req.set_instance(instance);
    req.set_data(data);
    client_.send(req);
}

void SerialProxy::get_modem_pins(const std::uint32_t instance, ModemPinsHandler once) {
    if (once) {
        ensure_modem_pins_handler();
        pending_modem_pins_[instance].push_back(std::move(once));
    }
    proto::SerialProxyGetModemPinsRequest req;
    req.set_instance(instance);
    client_.send(req);
}

void SerialProxy::set_modem_pins(const std::uint32_t instance,
                                 const SerialProxyLineStates lines) const {
    set_modem_pins_raw(instance, lines.to_bits());
}

void SerialProxy::set_modem_pins_raw(const std::uint32_t instance,
                                     const std::uint32_t line_states) const {
    proto::SerialProxySetModemPinsRequest req;
    req.set_instance(instance);
    req.set_line_states(line_states);
    client_.send(req);
}

}  // namespace esphome::api
