#pragma once

/// @file
/// @brief Connection: the ESPHome native-API protocol state machine. Owns the
///        transport + frame helper, drives the handshake, dispatches inbound
///        messages and runs keepalive. No entity semantics live here.

#include <esphome/api/bytes.hpp>
#include <esphome/api/connection/connection_options.hpp>
#include <esphome/api/connection/connection_state.hpp>
#include <esphome/api/frame/frame_helper.hpp>
#include <esphome/api/proto/proto_message.hpp>
#include <esphome/api/transport/executor.hpp>
#include <esphome/api/transport/transport.hpp>

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace esphome::api {

/// Information returned by the device in its HelloResponse.
struct ServerHello {
    int api_version_major = 0;
    int api_version_minor = 0;
    std::string server_info;
    std::string name;
};

/// Drives one logical session with an ESPHome device: TCP connect, optional
/// Noise handshake (via the injected frame helper), HelloRequest/HelloResponse,
/// keepalive pings and inbound message dispatch.
///
/// All methods must be invoked on the injected Executor (single-threaded from
/// the connection's point of view). Transports/timers post back onto it.
class Connection {
public:
    using MessageHandler = std::function<void(const ProtoMessage&)>;
    using RawHandler = std::function<void(std::uint32_t msg_type, ByteView payload)>;
    using StateHandler = std::function<void(ConnectionState)>;
    using ErrorHandler = std::function<void(std::error_code, const std::string&)>;
    using ConnectHandler = std::function<void(std::error_code)>;

    Connection(Executor& executor,
               std::unique_ptr<Transport> transport,
               std::unique_ptr<FrameHelper> frame_helper,
               ConnectionOptions options);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// Connect to `host`:`port`, perform the handshake and signal completion via
    /// `on_done` exactly once (empty error_code on success).
    void start(const std::string& host, std::uint16_t port, ConnectHandler on_done);

    /// Serialize and send a typed protobuf message.
    void send(const ProtoMessage& msg) const;

    /// Send a pre-serialized payload under an explicit message type id.
    void send_raw(std::uint32_t msg_type, ByteView payload) const;

    /// Install a handler for one message id (replaces any previous handler).
    void on(std::uint32_t msg_type, MessageHandler handler);

    /// Install a catch-all handler invoked for every decoded inbound message,
    /// after any id-specific handler.
    void on_any(MessageHandler handler);

    /// Install a handler invoked with the raw bytes of every inbound frame
    /// (even messages the registry cannot decode). Runs before decoding.
    void on_raw(RawHandler handler);

    /// Observe state transitions.
    void on_state(StateHandler handler);

    /// Observe terminal errors (read failure, protocol violation, timeout).
    void on_error(ErrorHandler handler);

    /// Begin a graceful disconnect (DisconnectRequest, then close).
    void disconnect();

    [[nodiscard]] ConnectionState state() const noexcept {
        return state_;
    }
    [[nodiscard]] const ServerHello& server_hello() const noexcept {
        return hello_;
    }
    [[nodiscard]] bool is_connected() const noexcept {
        return state_ == ConnectionState::Connected;
    }

    /// The exception that caused the most recent failure, if it carried richer
    /// type/message information than the connect handler's `std::error_code`
    /// (e.g. an EncryptionMismatchError or HandshakeError from the handshake).
    /// Null when the failure was a plain transport error or no failure occurred.
    [[nodiscard]] std::exception_ptr last_error() const noexcept {
        return last_error_;
    }

private:
    void set_state(ConnectionState state);
    void on_connected_transport(std::error_code ec);
    void on_bytes(std::error_code ec, ByteView data);
    void drain_frames();
    void handle_frame(std::uint32_t msg_type, ByteView payload);
    void handle_internal(std::uint32_t msg_type, const ProtoMessage& msg);
    void send_hello() const;
    void finish_connect(std::error_code ec);
    void fail(std::error_code ec, const std::string& what);

    void start_keepalive();
    void stop_keepalive();
    void send_ping();

    Executor& executor_;
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<FrameHelper> frame_helper_;
    ConnectionOptions options_;

    ConnectionState state_ = ConnectionState::Disconnected;
    ServerHello hello_;

    std::vector<MessageHandler> handlers_;  // indexed by message id
    MessageHandler any_handler_;
    RawHandler raw_handler_;
    StateHandler state_handler_;
    ErrorHandler error_handler_;
    ConnectHandler connect_handler_;
    std::exception_ptr last_error_;

    TimerId connect_timer_ = invalid_timer;
    TimerId ping_timer_ = invalid_timer;
    TimerId pong_timer_ = invalid_timer;
    bool awaiting_pong_ = false;
};

}  // namespace esphome::api
