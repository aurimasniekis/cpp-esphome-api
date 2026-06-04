// End-to-end test of the real stack: Asio TcpTransport + AsioExecutor +
// Connection + SyncClient, driven against a loopback POSIX server.

#include <esphome/api/exception.hpp>
#include <esphome/api/sync_client.hpp>

#include <gtest/gtest.h>

#include "support/loopback_server.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using esphome::api::ClientOptions;
using esphome::api::ConnectionError;
using esphome::api::SyncClient;
using esphome::api::testing::LoopbackServer;

namespace {

ClientOptions local_options(const std::uint16_t port) {
    ClientOptions opt;
    opt.host = "127.0.0.1";
    opt.port = port;
    opt.connection.connect_timeout = std::chrono::seconds(5);
    return opt;
}

// Bind an ephemeral port then release it, yielding a port nothing listens on.
std::uint16_t closed_port() {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("closed_port: socket() failed");
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        throw std::runtime_error("closed_port: bind() failed");
    }
    socklen_t len = sizeof(addr);
    ::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len);
    const std::uint16_t port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

}  // namespace

TEST(SyncClientIntegration, ConnectHandshakeAndDeviceInfo) {
    LoopbackServer server;
    server.device_name = "loopback-test";
    server.start();

    SyncClient client(local_options(server.port()));
    client.connect();

    EXPECT_TRUE(client.is_connected());
    EXPECT_EQ(client.server_hello().name, "loopback-test");
    EXPECT_EQ(client.server_hello().api_version_major, 1);

    const auto info = client.device_info();
    EXPECT_EQ(info.name, "loopback-test");
    EXPECT_EQ(info.model, "mock");

    client.disconnect();
    EXPECT_FALSE(client.is_connected());
}

TEST(SyncClientIntegration, ConnectionRefusedThrows) {
    SyncClient client(local_options(closed_port()));
    EXPECT_THROW(client.connect(), ConnectionError);
}

#if defined(ESPHOME_API_HAS_NOISE)
TEST(SyncClientIntegration, NoiseEncryptedSession) {
    LoopbackServer server;
    server.device_name = "secure-device";
    server.noise_psk_b64 = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=";
    server.start();

    ClientOptions opt = local_options(server.port());
    opt.connection.noise_psk = server.noise_psk_b64;
    const SyncClient client(opt);
    client.connect();

    EXPECT_TRUE(client.is_connected());
    EXPECT_EQ(client.server_hello().name, "secure-device");

    const auto info = client.device_info();
    EXPECT_EQ(info.name, "secure-device");
    EXPECT_TRUE(info.api_encryption_supported);

    client.disconnect();
}

TEST(SyncClientIntegration, NoiseWrongPskFails) {
    LoopbackServer server;
    server.noise_psk_b64 = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=";
    server.start();

    ClientOptions opt = local_options(server.port());
    opt.connection.noise_psk = "AQQHCg0QExYZHB8iJSgrLjE0Nzo9QENGSUxPUlVYW14=";  // wrong key
    const SyncClient client(opt);
    // The handshake rejection must propagate through connect() with its real
    // type and device message, not collapse into a generic "protocol error".
    try {
        client.connect();
        FAIL() << "expected the connect to throw";
    } catch (const esphome::api::HandshakeError& e) {
        EXPECT_NE(std::string(e.what()).find("handshake"), std::string::npos);
        EXPECT_EQ(std::string(e.what()).find("connect failed:"), std::string::npos);
    }
}
#endif
