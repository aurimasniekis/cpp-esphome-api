#include <esphome/api/connection/connection.hpp>
#include <esphome/api/exception.hpp>
#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include "api.pb.h"

#include <utility>

#include <google/protobuf/message.h>

namespace esphome::api {

namespace {

// API version this client advertises in HelloRequest. The device serves the
// connection at min(client, device). At API >= 1.14 the device no longer sends
// `object_id` (ESPHome PR #12698) — the client derives it from `name` instead
// (see object_id_from_name). The device negotiates down for older protocols; a
// major mismatch (> 2) is fatal.
constexpr int client_api_version_major = 1;
constexpr int client_api_version_minor = 14;

ByteBuffer serialize(const ProtoMessage& msg) {
    ByteBuffer out;
    const std::size_t n = msg.ByteSizeLong();
    out.resize(n);
    if (n != 0 && !msg.SerializeToArray(out.data(), static_cast<int>(n))) {
        throw ProtocolError("failed to serialize outgoing protobuf message");
    }
    return out;
}

std::error_code protocol_ec() {
    return std::make_error_code(std::errc::protocol_error);
}

}  // namespace

Connection::Connection(Executor& executor,
                       std::unique_ptr<Transport> transport,
                       std::unique_ptr<FrameHelper> frame_helper,
                       ConnectionOptions options)
    : executor_(executor), transport_(std::move(transport)), frame_helper_(std::move(frame_helper)),
      options_(std::move(options)), handlers_(message_id_max + 1) {}

Connection::~Connection() {
    stop_keepalive();
    if (connect_timer_ != invalid_timer) {
        executor_.cancel(connect_timer_);
    }
    if (transport_) {
        transport_->close();
    }
}

void Connection::set_state(const ConnectionState state) {
    if (state == state_) {
        return;
    }
    state_ = state;
    if (state_handler_) {
        state_handler_(state);
    }
}

void Connection::start(const std::string& host, const std::uint16_t port, ConnectHandler on_done) {
    connect_handler_ = std::move(on_done);
    last_error_ = nullptr;
    set_state(ConnectionState::Connecting);

    connect_timer_ = executor_.schedule_after(options_.connect_timeout, [this]() {
        if (state_ != ConnectionState::Connected) {
            fail(std::make_error_code(std::errc::timed_out), "handshake timed out");
        }
    });

    transport_->async_connect(
        host, port, [this](const std::error_code ec) { on_connected_transport(ec); });
}

void Connection::on_connected_transport(const std::error_code ec) {
    if (ec) {
        fail(ec, "tcp connect failed");
        return;
    }
    transport_->start_read(
        [this](const std::error_code read_ec, const ByteView data) { on_bytes(read_ec, data); });

    if (frame_helper_->needs_handshake()) {
        // Encrypted: send the first handshake flight and wait for the frame
        // helper to signal handshake completion before sending Hello.
        frame_helper_->set_handshake_handlers(
            [this]() {
                send_hello();
                set_state(ConnectionState::HelloSent);
            },
            [this](const std::string& what) {
                // The handshake reports failures as a plain string; wrap it as a
                // HandshakeError so the connect path surfaces the right type.
                last_error_ = std::make_exception_ptr(HandshakeError(what));
                fail(protocol_ec(), what);
            });
        if (ByteBuffer initial = frame_helper_->connect_bytes(); !initial.empty()) {
            transport_->async_write(std::move(initial), {});
        }
    } else {
        // Plaintext: proceed straight to Hello.
        send_hello();
        set_state(ConnectionState::HelloSent);
    }
}

void Connection::send_hello() {
    proto::HelloRequest hello;
    hello.set_client_info(options_.client_info);
    hello.set_api_version_major(client_api_version_major);
    hello.set_api_version_minor(client_api_version_minor);
    send(hello);

    // The device only marks the connection authenticated after a login. We send
    // the AuthenticationRequest right after Hello (as aioesphomeapi does, for
    // both plaintext and Noise) so subsequent requests aren't rejected with
    // "requested access without authentication". The password is empty unless
    // the device uses the deprecated plaintext API password.
    if (options_.login) {
        proto::AuthenticationRequest auth;
        if (!options_.password.empty()) {
            auth.set_password(options_.password);
        }
        send(auth);
    }
}

void Connection::on_bytes(const std::error_code ec, const ByteView data) {
    if (ec) {
        if (state_ == ConnectionState::Closing || state_ == ConnectionState::Closed) {
            return;  // expected during teardown
        }
        fail(ec, "read failed");
        return;
    }
    try {
        frame_helper_->feed(data);
        drain_frames();
    } catch (const ProtocolError& e) {
        // Preserve the concrete exception (type + message) so the connect path
        // can rethrow it instead of collapsing to a bare "protocol error".
        last_error_ = std::current_exception();
        fail(protocol_ec(), e.what());
    }
}

void Connection::drain_frames() {
    std::uint32_t type = 0;
    ByteView payload;
    while (frame_helper_->next(type, payload) == FrameStatus::Ok) {
        handle_frame(type, payload);
        if (state_ == ConnectionState::Failed || state_ == ConnectionState::Closed) {
            return;
        }
    }
}

void Connection::handle_frame(const std::uint32_t msg_type, const ByteView payload) {
    if (raw_handler_) {
        raw_handler_(msg_type, payload);
    }

    const std::unique_ptr<ProtoMessage> msg = MessageRegistry::instance().create(msg_type);
    if (!msg) {
        return;  // unknown id — already surfaced via raw_handler_, if any
    }
    if (!msg->ParseFromArray(payload.data(), static_cast<int>(payload.size()))) {
        fail(protocol_ec(), "failed to parse message payload");
        return;
    }

    handle_internal(msg_type, *msg);
    if (state_ == ConnectionState::Failed || state_ == ConnectionState::Closed) {
        return;
    }

    if (msg_type <= message_id_max && handlers_[msg_type]) {
        handlers_[msg_type](*msg);
    }
    if (any_handler_) {
        any_handler_(*msg);
    }
}

void Connection::handle_internal(std::uint32_t msg_type, const ProtoMessage& msg) {
    switch (static_cast<MessageId>(msg_type)) {
    case MessageId::HelloResponse: {
        if (state_ != ConnectionState::HelloSent) {
            return;
        }
        const auto& hr = static_cast<const proto::HelloResponse&>(msg);
        hello_.api_version_major = static_cast<int>(hr.api_version_major());
        hello_.api_version_minor = static_cast<int>(hr.api_version_minor());
        hello_.server_info = hr.server_info();
        hello_.name = hr.name();

        if (hello_.api_version_major > 2) {
            fail(protocol_ec(),
                 "unsupported API major version " + std::to_string(hello_.api_version_major));
            return;
        }
        if (!options_.expected_name.empty() && !hello_.name.empty() &&
            hello_.name != options_.expected_name) {
            fail(protocol_ec(),
                 "connected to '" + hello_.name + "' but expected '" + options_.expected_name +
                     "'");
            return;
        }
        if (connect_timer_ != invalid_timer) {
            executor_.cancel(connect_timer_);
            connect_timer_ = invalid_timer;
        }
        set_state(ConnectionState::Connected);
        start_keepalive();
        finish_connect(std::error_code{});
        break;
    }
    case MessageId::AuthenticationResponse: {
        if (const auto& ar = static_cast<const proto::AuthenticationResponse&>(msg);
            ar.invalid_password()) {
            fail(std::make_error_code(std::errc::permission_denied),
                 "authentication rejected: invalid password");
        }
        break;
    }
    case MessageId::DisconnectRequest: {
        const proto::DisconnectResponse resp;
        send(resp);
        stop_keepalive();
        transport_->close();
        set_state(ConnectionState::Closed);
        if (connect_handler_) {
            finish_connect(std::make_error_code(std::errc::connection_aborted));
        }
        break;
    }
    case MessageId::DisconnectResponse: {
        stop_keepalive();
        transport_->close();
        set_state(ConnectionState::Closed);
        break;
    }
    case MessageId::PingRequest: {
        const proto::PingResponse resp;
        send(resp);
        break;
    }
    case MessageId::PingResponse: {
        awaiting_pong_ = false;
        if (pong_timer_ != invalid_timer) {
            executor_.cancel(pong_timer_);
            pong_timer_ = invalid_timer;
        }
        break;
    }
    default:
        break;
    }
}

void Connection::send(const ProtoMessage& msg) const {
    const std::uint32_t id = MessageRegistry::id_of(msg);
    send_raw(id, serialize(msg));
}

void Connection::send_raw(const std::uint32_t msg_type, const ByteView payload) const {
    ByteBuffer wire;
    frame_helper_->encode(msg_type, payload, wire);
    transport_->async_write(std::move(wire), {});
}

void Connection::on(const std::uint32_t msg_type, MessageHandler handler) {
    if (msg_type <= message_id_max) {
        handlers_[msg_type] = std::move(handler);
    }
}

void Connection::on_any(MessageHandler handler) {
    any_handler_ = std::move(handler);
}

void Connection::on_raw(RawHandler handler) {
    raw_handler_ = std::move(handler);
}

void Connection::on_state(StateHandler handler) {
    state_handler_ = std::move(handler);
}

void Connection::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
}

void Connection::disconnect() {
    if (state_ != ConnectionState::Connected && state_ != ConnectionState::HelloSent) {
        return;
    }
    stop_keepalive();
    set_state(ConnectionState::Closing);

    const proto::DisconnectRequest req;
    ByteBuffer wire;
    frame_helper_->encode(MessageRegistry::id_of(req), serialize(req), wire);
    transport_->async_write(std::move(wire), [this](std::error_code /*ec*/) {
        transport_->close();
        set_state(ConnectionState::Closed);
    });
}

void Connection::finish_connect(const std::error_code ec) {
    if (connect_handler_) {
        const ConnectHandler handler = std::move(connect_handler_);
        connect_handler_ = nullptr;
        handler(ec);
    }
}

void Connection::fail(const std::error_code ec, const std::string& what) {
    if (state_ == ConnectionState::Failed || state_ == ConnectionState::Closed) {
        return;
    }
    stop_keepalive();
    if (connect_timer_ != invalid_timer) {
        executor_.cancel(connect_timer_);
        connect_timer_ = invalid_timer;
    }
    if (transport_) {
        transport_->close();
    }
    const bool was_connecting = static_cast<bool>(connect_handler_);
    set_state(ConnectionState::Failed);
    if (was_connecting) {
        finish_connect(ec);
    } else if (error_handler_) {
        error_handler_(ec, what);
    }
}

void Connection::start_keepalive() {
    if (options_.keepalive_interval.count() <= 0) {
        return;
    }
    ping_timer_ = executor_.schedule_after(options_.keepalive_interval, [this]() { send_ping(); });
}

void Connection::stop_keepalive() {
    if (ping_timer_ != invalid_timer) {
        executor_.cancel(ping_timer_);
        ping_timer_ = invalid_timer;
    }
    if (pong_timer_ != invalid_timer) {
        executor_.cancel(pong_timer_);
        pong_timer_ = invalid_timer;
    }
    awaiting_pong_ = false;
}

void Connection::send_ping() {
    if (state_ != ConnectionState::Connected) {
        return;
    }
    const proto::PingRequest ping;
    send(ping);

    if (!awaiting_pong_) {
        awaiting_pong_ = true;
        pong_timer_ = executor_.schedule_after(options_.keepalive_timeout, [this]() {
            pong_timer_ = invalid_timer;
            if (awaiting_pong_) {
                fail(std::make_error_code(std::errc::timed_out), "keepalive pong timed out");
            }
        });
    }
    ping_timer_ = executor_.schedule_after(options_.keepalive_interval, [this]() { send_ping(); });
}

}  // namespace esphome::api
