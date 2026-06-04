#include <esphome/api/exception.hpp>
#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>
#include <esphome/api/sync_client.hpp>

#include "api.pb.h"

#include <chrono>
#include <exception>
#include <utility>

namespace esphome::api {

namespace {
constexpr std::chrono::milliseconds pump_slice{20};
}  // namespace

SyncClient::SyncClient(const ClientOptions& options)
    : client_(options), subscribe_on_connect_(options.subscribe_on_connect),
      connect_pump_timeout_(options.connection.connect_timeout + std::chrono::seconds(5)) {}

void SyncClient::pump_until(const std::function<bool()>& predicate,
                            const std::chrono::milliseconds timeout) const {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            throw TimeoutError("operation timed out after " + std::to_string(timeout.count()) +
                               "ms");
        }
        // A work_guard keeps the loop alive, so run_for blocks for the full
        // slice when idle — no hot spin, and no restart() needed (the context
        // is never "stopped" here).
        (void)client_.run_for(pump_slice);
    }
}

void SyncClient::connect() const {
    std::error_code result;
    bool done = false;
    client_.async_connect([&](const std::error_code ec) {
        result = ec;
        done = true;
    });
    pump_until([&] { return done; }, connect_pump_timeout_);
    if (result) {
        // If the failure carried a richer exception (encryption mismatch, a
        // rejected handshake, ...), rethrow it so the caller sees the real type
        // and message rather than a generic "protocol error".
        if (const std::exception_ptr ex = client_.connection().last_error()) {
            std::rethrow_exception(ex);
        }
        throw ConnectionError("connect failed: " + result.message());
    }

    // The Client feeds its entity store automatically; on connect we enumerate
    // the entities and subscribe to states so entities() is live immediately.
    if (subscribe_on_connect_) {
        subscribe_states();
        list_entities();
    }
}

void SyncClient::list_entities() const {
    if (!client_.is_connected()) {
        throw ApiError("list_entities() called while not connected");
    }
    bool done = false;
    constexpr auto done_id = static_cast<std::uint32_t>(MessageId::ListEntitiesDoneResponse);
    client_.on(done_id, [&](const ProtoMessage&) { done = true; });

    const proto::ListEntitiesRequest req;
    client_.send(req);
    pump_until([&] { return done; }, request_timeout_);
    client_.on(done_id, nullptr);
}

SerialPort SyncClient::serial_proxy(const std::uint32_t instance) {
    return client_.serial_proxy(instance);
}

SerialPort SyncClient::serial_proxy(const std::string& name) {
    if (!client_.has_device_info()) {
        (void)device_info();  // fetch + cache so the name can be resolved
    }
    return client_.serial_proxy(name);
}

void SyncClient::subscribe_states() const {
    if (!client_.is_connected()) {
        throw ApiError("subscribe_states() called while not connected");
    }
    const proto::SubscribeStatesRequest req;
    client_.send(req);
}

void SyncClient::disconnect() const {
    if (client_.state() != ConnectionState::Connected &&
        client_.state() != ConnectionState::HelloSent) {
        return;
    }
    client_.disconnect();
    try {
        pump_until(
            [&] {
                const ConnectionState s = client_.state();
                return s == ConnectionState::Closed || s == ConnectionState::Failed ||
                       s == ConnectionState::Disconnected;
            },
            std::chrono::seconds(5));
    } catch (const TimeoutError&) {
        // Best-effort close; the socket is torn down regardless.
    }
}

DeviceInfo SyncClient::device_info() const {
    if (!client_.is_connected()) {
        throw ApiError("device_info() called while not connected");
    }
    DeviceInfo info;
    bool done = false;
    constexpr auto id = static_cast<std::uint32_t>(MessageId::DeviceInfoResponse);
    client_.on(id, [&](const ProtoMessage& msg) {
        info = DeviceInfo::from_proto(static_cast<const proto::DeviceInfoResponse&>(msg));
        done = true;
    });

    const proto::DeviceInfoRequest req;
    client_.send(req);
    pump_until([&] { return done; }, request_timeout_);

    client_.on(id, nullptr);
    return info;
}

}  // namespace esphome::api
