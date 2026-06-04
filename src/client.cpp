#include <esphome/api/client.hpp>
#include <esphome/api/exception.hpp>
#include <esphome/api/frame/plaintext_frame_helper.hpp>
#include <esphome/api/model/device_info.hpp>
#include <esphome/api/model/entity_registry.hpp>
#include <esphome/api/model/entity_store.hpp>
#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>
#include <esphome/api/subsystems/bluetooth_proxy.hpp>
#include <esphome/api/subsystems/home_assistant_services.hpp>
#include <esphome/api/subsystems/log_stream.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>
#include <esphome/api/subsystems/voice_assistant.hpp>
#include <esphome/api/subsystems/zwave_proxy.hpp>

#if defined(ESPHOME_API_HAS_NOISE)
#include <esphome/api/frame/noise_frame_helper.hpp>
#endif

#include "api.pb.h"
#include "transport/asio_executor.hpp"
#include "transport/tcp_transport.hpp"
#include <asio.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace esphome::api {

struct Client::Impl {
    explicit Impl(ClientOptions opts)
        : options(std::move(opts)), work_guard(asio::make_work_guard(io)), executor(io) {
        auto transport = std::make_unique<TcpTransport>(io.get_executor());
        std::unique_ptr<FrameHelper> frame = make_frame_helper();
        connection = std::make_unique<Connection>(
            executor, std::move(transport), std::move(frame), options.connection);

        // Feed every inbound message into the entity store, then fan out to any
        // user-registered catch-all handlers.
        connection->on_any([this](const ProtoMessage& msg) {
            store.ingest(msg);
            if (MessageRegistry::id_of(msg) ==
                static_cast<std::uint32_t>(MessageId::DeviceInfoResponse)) {
                device_info =
                    DeviceInfo::from_proto(static_cast<const proto::DeviceInfoResponse&>(msg));
                device_info_loaded = true;
            }
            for (auto& handler : any_handlers) {
                handler(msg);
            }
        });
    }

    std::unique_ptr<FrameHelper> make_frame_helper() const {
        if (options.connection.noise_psk.empty()) {
            return std::make_unique<PlaintextFrameHelper>();
        }
#if defined(ESPHOME_API_HAS_NOISE)
        NoiseConfig config;
        config.psk_base64 = options.connection.noise_psk;
        config.expected_name = options.connection.expected_name;
        return std::make_unique<NoiseFrameHelper>(config);
#else
        throw ApiError("a Noise PSK was provided but the library was built without "
                       "Noise support (ESPHOME_API_WITH_NOISE=OFF)");
#endif
    }

    ClientOptions options;
    asio::io_context io;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
    AsioExecutor executor;
    std::unique_ptr<Connection> connection;
    EntityStore store;
    DeviceInfo device_info;
    bool device_info_loaded = false;
    std::vector<MessageHandler> any_handlers;

    // Subsystems, created on first access.
    std::unique_ptr<BluetoothProxy> bluetooth;
    std::unique_ptr<LogStream> logs;
    std::unique_ptr<HomeAssistantServices> home_assistant;
    std::unique_ptr<VoiceAssistant> voice;
    std::unique_ptr<ZWaveProxy> zwave;
    std::unique_ptr<SerialProxy> serial;
};

Client::Client(ClientOptions options) : impl_(std::make_unique<Impl>(std::move(options))) {}

Client::~Client() = default;

void Client::async_connect(ConnectHandler on_done) const {
    impl_->connection->start(impl_->options.host, impl_->options.port, std::move(on_done));
}

void Client::disconnect() const {
    impl_->connection->disconnect();
}

ConnectionState Client::state() const {
    return impl_->connection->state();
}

bool Client::is_connected() const {
    return impl_->connection->is_connected();
}

const ServerHello& Client::server_hello() const {
    return impl_->connection->server_hello();
}

void Client::send(const ProtoMessage& msg) const {
    impl_->connection->send(msg);
}

void Client::send_raw(const std::uint32_t msg_type, const ByteView payload) const {
    impl_->connection->send_raw(msg_type, payload);
}

void Client::on(const std::uint32_t msg_type, MessageHandler handler) const {
    impl_->connection->on(msg_type, std::move(handler));
}

void Client::on_any(MessageHandler handler) const {
    // The connection's single any-slot is owned by the store feed; user handlers
    // are multiplexed here.
    impl_->any_handlers.push_back(std::move(handler));
}

void Client::on_raw(RawHandler handler) const {
    impl_->connection->on_raw(std::move(handler));
}

void Client::on_state(StateHandler handler) const {
    impl_->connection->on_state(std::move(handler));
}

void Client::on_error(ErrorHandler handler) const {
    impl_->connection->on_error(std::move(handler));
}

void Client::run() const {
    impl_->io.run();
}

std::size_t Client::run_for(const std::chrono::milliseconds duration) const {
    return impl_->io.run_for(duration);
}

std::size_t Client::run_one() const {
    return impl_->io.run_one();
}

std::size_t Client::poll() const {
    return impl_->io.poll();
}

void Client::stop() const {
    impl_->io.stop();
}

void Client::restart() const {
    impl_->io.restart();
}

void Client::post(std::function<void()> fn) const {
    asio::post(impl_->io, std::move(fn));
}

Connection& Client::connection() const {
    return *impl_->connection;
}

EntityStore& Client::store() {
    return impl_->store;
}

const EntityStore& Client::store() const {
    return impl_->store;
}

EntityRegistry Client::entities() {
    return EntityRegistry(*this);
}

void Client::request_entity_list() const {
    const proto::ListEntitiesRequest req;
    send(req);
}

void Client::subscribe_to_states() const {
    const proto::SubscribeStatesRequest req;
    send(req);
}

BluetoothProxy& Client::bluetooth() {
    if (!impl_->bluetooth) {
        impl_->bluetooth = std::make_unique<BluetoothProxy>(*this);
    }
    return *impl_->bluetooth;
}

LogStream& Client::logs() {
    if (!impl_->logs) {
        impl_->logs = std::make_unique<LogStream>(*this);
    }
    return *impl_->logs;
}

HomeAssistantServices& Client::home_assistant() {
    if (!impl_->home_assistant) {
        impl_->home_assistant = std::make_unique<HomeAssistantServices>(*this);
    }
    return *impl_->home_assistant;
}

VoiceAssistant& Client::voice() {
    if (!impl_->voice) {
        impl_->voice = std::make_unique<VoiceAssistant>(*this);
    }
    return *impl_->voice;
}

ZWaveProxy& Client::zwave() {
    if (!impl_->zwave) {
        impl_->zwave = std::make_unique<ZWaveProxy>(*this);
    }
    return *impl_->zwave;
}

SerialProxy& Client::serial() {
    if (!impl_->serial) {
        impl_->serial = std::make_unique<SerialProxy>(*this);
    }
    return *impl_->serial;
}

SerialPort Client::serial_proxy(const std::uint32_t instance) {
    return serial().instance(instance);
}

SerialPort Client::serial_proxy(const std::string& name) {
    if (!impl_->device_info_loaded) {
        throw ApiError("serial_proxy(\"" + name +
                       "\"): device info not received yet — call device_info() first");
    }
    const auto& ports = impl_->device_info.serial_proxies;
    for (std::size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].name == name) {
            return serial().instance(static_cast<std::uint32_t>(i));
        }
    }
    throw ApiError("no serial proxy named '" + name + "'");
}

bool Client::has_device_info() const {
    return impl_->device_info_loaded;
}

const DeviceInfo& Client::device_info() const {
    return impl_->device_info;
}

}  // namespace esphome::api
