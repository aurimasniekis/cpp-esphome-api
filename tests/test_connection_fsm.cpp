#include <esphome/api/connection/connection.hpp>
#include <esphome/api/frame/plaintext_frame_helper.hpp>
#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include <gtest/gtest.h>

#include "api.pb.h"
#include "support/manual_executor.hpp"
#include "support/mock_transport.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <google/protobuf/message.h>

using esphome::api::ByteBuffer;
using esphome::api::ByteView;
using esphome::api::Connection;
using esphome::api::ConnectionOptions;
using esphome::api::ConnectionState;
using esphome::api::FrameStatus;
using esphome::api::MessageId;
using esphome::api::MessageRegistry;
using esphome::api::PlaintextFrameHelper;
using esphome::api::ProtoMessage;
using esphome::api::testing::ManualExecutor;
using esphome::api::testing::MockTransport;
namespace proto = esphome::api::proto;

namespace {

ByteBuffer serialize(const ProtoMessage& msg) {
    ByteBuffer out;
    const std::size_t n = msg.ByteSizeLong();
    out.resize(n);
    if (n != 0) {
        static_cast<void>(msg.SerializeToArray(out.data(), static_cast<int>(n)));
    }
    return out;
}

ByteBuffer encode_server(const ProtoMessage& msg) {
    PlaintextFrameHelper fh;
    ByteBuffer wire;
    fh.encode(MessageRegistry::id_of(msg), serialize(msg), wire);
    return wire;
}

struct SentFrame {
    std::uint32_t type;
    ByteBuffer payload;
};

std::vector<SentFrame> decode_sent(const ByteBuffer& sent) {
    PlaintextFrameHelper fh;
    fh.feed(sent);
    std::vector<SentFrame> frames;
    std::uint32_t type = 0;
    ByteView view;
    while (fh.next(type, view) == FrameStatus::Ok) {
        frames.push_back({type, ByteBuffer(view.begin(), view.end())});
    }
    return frames;
}

bool sent_contains(const ByteBuffer& sent, MessageId id) {
    const auto frames = decode_sent(sent);
    return std::any_of(frames.begin(), frames.end(), [id](const auto& f) {
        return f.type == static_cast<std::uint32_t>(id);
    });
}

}  // namespace

class ConnectionFsm : public ::testing::Test {
protected:
    void make(ConnectionOptions options = {}) {
        auto transport = std::make_unique<MockTransport>();
        mock = transport.get();
        auto frame = std::make_unique<PlaintextFrameHelper>();
        conn = std::make_unique<Connection>(
            exec, std::move(transport), std::move(frame), std::move(options));
        conn->on_error([this](const std::error_code ec, const std::string& what) {
            error_ec = ec;
            error_what = what;
        });
    }

    void start() {
        conn->start("device.local", 6053, [this](const std::error_code ec) {
            connect_ec = ec;
            connect_done = true;
        });
        exec.drain();
    }

    void reach_connected(const std::string& name = "test-device") {
        start();
        mock->complete_connect();
        exec.drain();
        proto::HelloResponse hr;
        hr.set_api_version_major(1);
        hr.set_api_version_minor(10);
        hr.set_name(name);
        mock->deliver(encode_server(hr));
        exec.drain();
    }

    ManualExecutor exec;
    MockTransport* mock = nullptr;
    std::unique_ptr<Connection> conn;

    std::error_code connect_ec;
    bool connect_done = false;
    std::error_code error_ec;
    std::string error_what;
};

TEST_F(ConnectionFsm, ConnectRequestsHostAndPort) {
    make();
    start();
    EXPECT_TRUE(mock->connect_requested);
    EXPECT_EQ(mock->host, "device.local");
    EXPECT_EQ(mock->port, 6053);
    EXPECT_EQ(conn->state(), ConnectionState::Connecting);
}

TEST_F(ConnectionFsm, SendsHelloAfterTcpConnect) {
    make();
    start();
    mock->complete_connect();
    exec.drain();

    EXPECT_EQ(conn->state(), ConnectionState::HelloSent);
    const auto frames = decode_sent(mock->sent);
    ASSERT_FALSE(frames.empty());
    EXPECT_EQ(frames[0].type, static_cast<std::uint32_t>(MessageId::HelloRequest));

    proto::HelloRequest hello;
    ASSERT_TRUE(
        hello.ParseFromArray(frames[0].payload.data(), static_cast<int>(frames[0].payload.size())));
    EXPECT_EQ(hello.client_info(), "esphome-api-client");
    EXPECT_EQ(hello.api_version_major(), 1U);

    // The login (AuthenticationRequest) must follow Hello so the device marks
    // the connection authenticated.
    EXPECT_TRUE(sent_contains(mock->sent, MessageId::AuthenticationRequest));
}

TEST_F(ConnectionFsm, InvalidPasswordFails) {
    ConnectionOptions opt;
    opt.password = "wrong";
    make(opt);
    reach_connected();
    EXPECT_EQ(conn->state(), ConnectionState::Connected);

    proto::AuthenticationResponse resp;
    resp.set_invalid_password(true);
    mock->deliver(encode_server(resp));
    exec.drain();

    EXPECT_EQ(conn->state(), ConnectionState::Failed);
    EXPECT_EQ(error_ec, std::errc::permission_denied);
}

TEST_F(ConnectionFsm, ReachesConnectedOnHelloResponse) {
    make();
    reach_connected("living-room");

    EXPECT_TRUE(connect_done);
    EXPECT_FALSE(connect_ec);
    EXPECT_EQ(conn->state(), ConnectionState::Connected);
    EXPECT_TRUE(conn->is_connected());
    EXPECT_EQ(conn->server_hello().name, "living-room");
    EXPECT_EQ(conn->server_hello().api_version_major, 1);
}

TEST_F(ConnectionFsm, RejectsUnsupportedMajorVersion) {
    make();
    start();
    mock->complete_connect();
    exec.drain();

    proto::HelloResponse hr;
    hr.set_api_version_major(3);
    mock->deliver(encode_server(hr));
    exec.drain();

    EXPECT_TRUE(connect_done);
    EXPECT_TRUE(connect_ec);
    EXPECT_EQ(conn->state(), ConnectionState::Failed);
}

TEST_F(ConnectionFsm, RejectsWrongDeviceName) {
    ConnectionOptions opt;
    opt.expected_name = "kitchen";
    make(opt);
    start();
    mock->complete_connect();
    exec.drain();

    proto::HelloResponse hr;
    hr.set_api_version_major(1);
    hr.set_name("bedroom");
    mock->deliver(encode_server(hr));
    exec.drain();

    EXPECT_TRUE(connect_ec);
    EXPECT_EQ(conn->state(), ConnectionState::Failed);
}

TEST_F(ConnectionFsm, TcpConnectFailurePropagates) {
    make();
    start();
    mock->complete_connect(std::make_error_code(std::errc::connection_refused));
    exec.drain();

    EXPECT_TRUE(connect_done);
    EXPECT_EQ(connect_ec, std::errc::connection_refused);
    EXPECT_EQ(conn->state(), ConnectionState::Failed);
}

TEST_F(ConnectionFsm, HandshakeTimeoutFails) {
    ConnectionOptions opt;
    opt.connect_timeout = std::chrono::milliseconds(5000);
    make(opt);
    start();
    // Never complete the connect; advance past the deadline.
    exec.advance(std::chrono::milliseconds(5000));

    EXPECT_TRUE(connect_done);
    EXPECT_EQ(connect_ec, std::errc::timed_out);
    EXPECT_EQ(conn->state(), ConnectionState::Failed);
}

TEST_F(ConnectionFsm, KeepalivePingThenPong) {
    ConnectionOptions opt;
    opt.keepalive_interval = std::chrono::milliseconds(20000);
    opt.keepalive_timeout = std::chrono::milliseconds(90000);
    make(opt);
    reach_connected();
    mock->sent.clear();

    exec.advance(std::chrono::milliseconds(20000));
    EXPECT_TRUE(sent_contains(mock->sent, MessageId::PingRequest));

    // Pong arrives → no failure even after the timeout window elapses.
    const proto::PingResponse pong;
    mock->deliver(encode_server(pong));
    exec.advance(std::chrono::milliseconds(90000));
    EXPECT_EQ(conn->state(), ConnectionState::Connected);
}

TEST_F(ConnectionFsm, KeepalivePongTimeoutFails) {
    ConnectionOptions opt;
    opt.keepalive_interval = std::chrono::milliseconds(20000);
    opt.keepalive_timeout = std::chrono::milliseconds(90000);
    make(opt);
    reach_connected();

    exec.advance(std::chrono::milliseconds(20000));  // ping sent
    exec.advance(std::chrono::milliseconds(90000));  // no pong → dead

    EXPECT_EQ(conn->state(), ConnectionState::Failed);
    EXPECT_EQ(error_ec, std::errc::timed_out);
}

TEST_F(ConnectionFsm, AnswersServerPing) {
    make();
    reach_connected();
    mock->sent.clear();

    const proto::PingRequest ping;
    mock->deliver(encode_server(ping));
    exec.drain();

    EXPECT_TRUE(sent_contains(mock->sent, MessageId::PingResponse));
}

TEST_F(ConnectionFsm, ServerDisconnectRequestClosesCleanly) {
    make();
    reach_connected();
    mock->sent.clear();

    const proto::DisconnectRequest req;
    mock->deliver(encode_server(req));
    exec.drain();

    EXPECT_TRUE(sent_contains(mock->sent, MessageId::DisconnectResponse));
    EXPECT_EQ(conn->state(), ConnectionState::Closed);
    EXPECT_TRUE(mock->closed);
}

TEST_F(ConnectionFsm, ClientDisconnectSendsRequestAndCloses) {
    make();
    reach_connected();
    mock->sent.clear();

    conn->disconnect();
    exec.drain();

    EXPECT_TRUE(sent_contains(mock->sent, MessageId::DisconnectRequest));
    EXPECT_EQ(conn->state(), ConnectionState::Closed);
    EXPECT_TRUE(mock->closed);
}

TEST_F(ConnectionFsm, DispatchesToRegisteredHandler) {
    make();
    reach_connected();

    int calls = 0;
    std::string got_name;
    conn->on(static_cast<std::uint32_t>(MessageId::DeviceInfoResponse),
             [&](const ProtoMessage& msg) {
                 ++calls;
                 got_name = static_cast<const proto::DeviceInfoResponse&>(msg).name();
             });

    proto::DeviceInfoResponse info;
    info.set_name("my-esp");
    mock->deliver(encode_server(info));
    exec.drain();

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(got_name, "my-esp");
}

TEST_F(ConnectionFsm, RawHandlerSeesEveryFrame) {
    make();
    reach_connected();

    std::vector<std::uint32_t> ids;
    conn->on_raw([&](const std::uint32_t type, ByteView) { ids.push_back(type); });

    const proto::PingRequest ping;
    mock->deliver(encode_server(ping));
    exec.drain();

    ASSERT_FALSE(ids.empty());
    EXPECT_EQ(ids.back(), static_cast<std::uint32_t>(MessageId::PingRequest));
}

TEST_F(ConnectionFsm, ReadErrorAfterConnectedFails) {
    make();
    reach_connected();

    mock->deliver_error(std::make_error_code(std::errc::connection_reset));
    exec.drain();

    EXPECT_EQ(conn->state(), ConnectionState::Failed);
    EXPECT_EQ(error_ec, std::errc::connection_reset);
}
