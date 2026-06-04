#pragma once

/// @file
/// @brief Blocking convenience facade over the async Client. Each call pumps the
///        event loop until the operation resolves or a deadline elapses.

#include <esphome/api/client.hpp>
#include <esphome/api/client_options.hpp>
#include <esphome/api/connection/connection.hpp>
#include <esphome/api/connection/connection_state.hpp>
#include <esphome/api/model/device_info.hpp>
#include <esphome/api/model/entity_registry.hpp>
#include <esphome/api/model/entity_store.hpp>

#include <chrono>
#include <functional>

namespace esphome::api {

/// Synchronous, single-threaded client. Construct, `connect()`, issue blocking
/// requests, then `disconnect()`. Not thread-safe.
class SyncClient {
public:
    explicit SyncClient(const ClientOptions& options);

    /// Connect and complete the handshake, blocking until ready. Throws
    /// ConnectionError / TimeoutError on failure.
    void connect() const;

    /// Gracefully disconnect, blocking briefly for the close to settle.
    void disconnect() const;

    [[nodiscard]] bool is_connected() const {
        return client_.is_connected();
    }
    [[nodiscard]] ConnectionState state() const {
        return client_.state();
    }
    [[nodiscard]] const ServerHello& server_hello() const {
        return client_.server_hello();
    }

    /// Request the device's DeviceInfoResponse and return it as a typed struct.
    DeviceInfo device_info() const;

    /// Request the full entity list and block until ListEntitiesDoneResponse.
    /// The entities are available afterwards via entities().
    void list_entities() const;

    /// Subscribe to live state updates. They are applied to entities() as the
    /// loop is pumped (e.g. inside pump_until or run_for on the async client).
    void subscribe_states() const;

    /// Object-oriented view of the discovered entities:
    /// `entities().switches()`, `entities().light("ceiling").turn_on()`, etc.
    [[nodiscard]] EntityRegistry entities() {
        return client_.entities();
    }
    /// The underlying entity store (raw typed info/state access).
    [[nodiscard]] EntityStore& store() noexcept {
        return client_.store();
    }
    [[nodiscard]] const EntityStore& store() const noexcept {
        return client_.store();
    }

    // --- Subsystems (delegated to the async client) --------------------------
    [[nodiscard]] BluetoothProxy& bluetooth() {
        return client_.bluetooth();
    }
    [[nodiscard]] LogStream& logs() {
        return client_.logs();
    }
    [[nodiscard]] HomeAssistantServices& home_assistant() {
        return client_.home_assistant();
    }
    [[nodiscard]] VoiceAssistant& voice() {
        return client_.voice();
    }
    [[nodiscard]] ZWaveProxy& zwave() {
        return client_.zwave();
    }
    [[nodiscard]] SerialProxy& serial() {
        return client_.serial();
    }
    /// Instance-bound serial-proxy handle: `sync.serial_proxy(0).write(...)`.
    [[nodiscard]] SerialPort serial_proxy(std::uint32_t instance);

    /// Serial-proxy handle by advertised name. Fetches device info on first use
    /// to resolve the name; throws ApiError if the name is unknown.
    [[nodiscard]] SerialPort serial_proxy(const std::string& name);

    /// Default per-request deadline (does not apply to `connect`, which uses the
    /// connection's own handshake timeout).
    void set_request_timeout(const std::chrono::milliseconds timeout) {
        request_timeout_ = timeout;
    }

    /// Pump the loop until `predicate` is true or `timeout` elapses. Exposed for
    /// building custom blocking request/response flows on top of `async()`.
    void pump_until(const std::function<bool()>& predicate,
                    std::chrono::milliseconds timeout) const;

    /// Access the underlying async client (register handlers, send raw, etc.).
    [[nodiscard]] Client& async() {
        return client_;
    }

private:
    Client client_;
    bool subscribe_on_connect_ = true;
    std::chrono::milliseconds request_timeout_{30000};
    std::chrono::milliseconds connect_pump_timeout_;
};

}  // namespace esphome::api
