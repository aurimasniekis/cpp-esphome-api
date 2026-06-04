#pragma once

/// @file
/// @brief Asynchronous orchestrator: owns the event loop and a Connection, and
///        exposes connect/send/subscribe plus loop-pumping primitives. Asio is
///        hidden behind a PIMPL so this header stays dependency-light.

#include <esphome/api/bytes.hpp>
#include <esphome/api/client_options.hpp>
#include <esphome/api/connection/connection.hpp>
#include <esphome/api/connection/connection_state.hpp>
#include <esphome/api/proto/proto_message.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <system_error>

namespace esphome::api {

struct DeviceInfo;
class EntityStore;
class EntityRegistry;
class BluetoothProxy;
class LogStream;
class HomeAssistantServices;
class VoiceAssistant;
class ZWaveProxy;
class SerialProxy;
class SerialPort;

/// Async client for a single device. Not thread-safe: drive it from one thread
/// (or post work onto its executor with `post`). SyncClient wraps this with
/// blocking convenience methods.
class Client {
public:
    using ConnectHandler = std::function<void(std::error_code)>;
    using MessageHandler = std::function<void(const ProtoMessage&)>;
    using RawHandler = std::function<void(std::uint32_t msg_type, ByteView payload)>;
    using StateHandler = std::function<void(ConnectionState)>;
    using ErrorHandler = std::function<void(std::error_code, const std::string&)>;

    explicit Client(ClientOptions options);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // --- Connection lifecycle ------------------------------------------------
    /// Begin connecting + handshaking; `on_done` fires once when the session is
    /// ready (or with an error). Must be driven by the event loop afterwards.
    void async_connect(ConnectHandler on_done) const;

    /// Begin a graceful disconnect.
    void disconnect() const;

    [[nodiscard]] ConnectionState state() const;
    [[nodiscard]] bool is_connected() const;
    [[nodiscard]] const ServerHello& server_hello() const;

    // --- Messaging -----------------------------------------------------------
    void send(const ProtoMessage& msg) const;
    void send_raw(std::uint32_t msg_type, ByteView payload) const;

    void on(std::uint32_t msg_type, MessageHandler handler) const;
    void on_any(MessageHandler handler) const;
    void on_raw(RawHandler handler) const;
    void on_state(StateHandler handler) const;
    void on_error(ErrorHandler handler) const;

    // --- Event loop ----------------------------------------------------------
    /// Run the loop until stopped (blocking). Typically used on a worker thread.
    void run() const;
    /// Run ready handlers for up to `duration`; returns handlers executed.
    std::size_t run_for(std::chrono::milliseconds duration) const;
    /// Run a single handler, blocking until one is available; returns count.
    std::size_t run_one() const;
    /// Execute any ready handlers without blocking; returns handlers executed.
    std::size_t poll() const;
    /// Stop the loop (causes `run` to return).
    void stop() const;
    /// Reset a stopped loop so it can run again.
    void restart() const;
    /// Post `fn` to run on the client's executor.
    void post(std::function<void()> fn) const;

    /// Access the underlying Connection (advanced/raw use).
    [[nodiscard]] Connection& connection() const;

    // --- Entities ------------------------------------------------------------
    /// The store of discovered entities and their latest states (auto-populated
    /// from every inbound message).
    [[nodiscard]] EntityStore& store();
    [[nodiscard]] const EntityStore& store() const;

    /// Typed, object-oriented view over the entity store: `entities().switches()`,
    /// `entities().light("ceiling")`, etc.
    [[nodiscard]] EntityRegistry entities();

    /// Ask the device to enumerate its entities (ListEntitiesRequest).
    void request_entity_list() const;
    /// Ask the device to stream state updates (SubscribeStatesRequest).
    void subscribe_to_states() const;

    // --- Subsystems (lazily created, owned by the client) --------------------
    [[nodiscard]] BluetoothProxy& bluetooth();
    [[nodiscard]] LogStream& logs();
    [[nodiscard]] HomeAssistantServices& home_assistant();
    [[nodiscard]] VoiceAssistant& voice();
    [[nodiscard]] ZWaveProxy& zwave();
    [[nodiscard]] SerialProxy& serial();

    /// An instance-bound serial-proxy handle: `client.serial_proxy(0).write(...)`.
    [[nodiscard]] SerialPort serial_proxy(std::uint32_t instance);

    /// Look up a serial-proxy handle by its advertised name. Requires the
    /// device's DeviceInfo to have been received (the client caches it); throws
    /// ApiError if absent or the name is unknown.
    [[nodiscard]] SerialPort serial_proxy(const std::string& name);

    /// The most recently received DeviceInfo (populated when a DeviceInfoResponse
    /// is seen). `has_device_info()` reports whether it is valid yet.
    [[nodiscard]] bool has_device_info() const;
    [[nodiscard]] const DeviceInfo& device_info() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace esphome::api
