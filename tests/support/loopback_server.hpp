#pragma once

/// @file
/// @brief Minimal in-process ESPHome device emulator over a real loopback TCP
///        socket, used to exercise the actual Asio transport + Connection +
///        SyncClient stack. Supports plaintext and Noise (POSIX only).

#include <esphome/api/bytes.hpp>
#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/crypto/noise_handshake.hpp>
#include <esphome/api/crypto/primitives.hpp>
#include <esphome/api/frame/plaintext_frame_helper.hpp>
#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/proto/message_registry.hpp>

#include "api.pb.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace esphome::api::testing {

namespace detail {

inline void append_u16(ByteBuffer& out, const std::size_t v) {
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

}  // namespace detail

/// Single-client device emulator. Answers Hello / DeviceInfo / Ping /
/// Disconnect. If `noise_psk_b64` is set, it runs the NNpsk0 responder.
class LoopbackServer {
public:
    std::string device_name = "loopback-device";
    std::string noise_psk_b64;  // empty ⇒ plaintext

    LoopbackServer() : listen_fd_(::socket(AF_INET, SOCK_STREAM, 0)) {
        if (listen_fd_ < 0) {
            throw std::runtime_error("loopback: socket() failed");
        }
        constexpr int one = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
            ::listen(listen_fd_, 1) != 0) {
            throw std::runtime_error("loopback: bind/listen failed");
        }
        socklen_t len = sizeof(addr);
        ::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
    }

    ~LoopbackServer() {
        stop();
    }

    LoopbackServer(const LoopbackServer&) = delete;
    LoopbackServer& operator=(const LoopbackServer&) = delete;

    [[nodiscard]] std::uint16_t port() const {
        return port_;
    }

    void start() {
        thread_ = std::thread([this] { serve(); });
    }

    void stop() {
        if (listen_fd_ >= 0) {
            ::shutdown(listen_fd_, SHUT_RDWR);
            ::close(listen_fd_);
            listen_fd_ = -1;
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    static ByteBuffer serialize(const ProtoMessage& msg) {
        ByteBuffer out;
        const std::size_t n = msg.ByteSizeLong();
        out.resize(n);
        if (n != 0) {
            static_cast<void>(msg.SerializeToArray(out.data(), static_cast<int>(n)));
        }
        return out;
    }

    void send(const int fd, const ProtoMessage& msg) {
        const std::uint32_t id = MessageRegistry::id_of(msg);
        const ByteBuffer payload = serialize(msg);
        ByteBuffer wire;
        if (noise_ready_) {
            ByteBuffer inner;
            detail::append_u16(inner, id);
            detail::append_u16(inner, payload.size());
            inner.insert(inner.end(), payload.begin(), payload.end());
            const ByteBuffer ct = send_cipher_.encrypt_with_ad(ByteView{}, inner);
            wire.push_back(0x01);
            detail::append_u16(wire, ct.size());
            wire.insert(wire.end(), ct.begin(), ct.end());
        } else {
            PlaintextFrameHelper fh;
            fh.encode(id, payload, wire);
        }
        ::send(fd, wire.data(), wire.size(), 0);
    }

    // Read one [0x01][u16][payload] frame, blocking. False on EOF.
    static bool recv_noise_frame(const int fd, ByteBuffer& buf, ByteBuffer& out_payload) {
        for (;;) {
            if (buf.size() >= 3) {
                if (const std::size_t len = (static_cast<std::size_t>(buf[1]) << 8) | buf[2];
                    buf.size() >= 3 + len) {
                    out_payload.assign(buf.begin() + 3,
                                       buf.begin() + 3 + static_cast<std::ptrdiff_t>(len));
                    buf.erase(buf.begin(), buf.begin() + 3 + static_cast<std::ptrdiff_t>(len));
                    return true;
                }
            }
            std::array<std::uint8_t, 4096> tmp{};
            const ssize_t n = ::recv(fd, tmp.data(), tmp.size(), 0);
            if (n <= 0) {
                return false;
            }
            buf.insert(buf.end(), tmp.data(), tmp.data() + n);
        }
    }

    bool noise_handshake(int fd, ByteBuffer& buf) {
        ByteBuffer decoded;
        noise::base64_decode(noise_psk_b64, decoded);
        noise::SymmetricKey psk{};
        for (std::size_t i = 0; i < psk.size() && i < decoded.size(); ++i) {
            psk[i] = decoded[i];
        }
        static const ByteBuffer prologue = {
            'N', 'o', 'i', 's', 'e', 'A', 'P', 'I', 'I', 'n', 'i', 't', 0, 0};
        noise::NoiseHandshake hs;
        hs.initialize(noise::NoiseHandshake::Role::Responder,
                      psk,
                      ByteView(prologue.data(), prologue.size()));

        if (ByteBuffer hello_frame; !recv_noise_frame(fd, buf, hello_frame)) {
            return false;
        }
        ByteBuffer handshake_frame;
        if (!recv_noise_frame(fd, buf, handshake_frame) || handshake_frame.empty()) {
            return false;
        }
        const ByteView e_msg(handshake_frame.data() + 1, handshake_frame.size() - 1);

        // Server hello: [proto=0x01][name\0].
        ByteBuffer hello_payload;
        hello_payload.push_back(0x01);
        hello_payload.insert(hello_payload.end(), device_name.begin(), device_name.end());
        hello_payload.push_back(0x00);
        ByteBuffer out;
        out.push_back(0x01);
        detail::append_u16(out, hello_payload.size());
        out.insert(out.end(), hello_payload.begin(), hello_payload.end());

        ByteBuffer hs_payload;
        ByteBuffer e_copy(e_msg.begin(), e_msg.end());
        if (ByteBuffer got; hs.read_message(ByteView(e_copy.data(), e_copy.size()), got)) {
            hs_payload.push_back(0x00);
            const ByteBuffer reply = hs.write_message();
            hs_payload.insert(hs_payload.end(), reply.begin(), reply.end());
            hs.split(send_cipher_, recv_cipher_);
            noise_ready_ = true;
        } else {
            hs_payload.push_back(0x01);
            const std::string err = "Handshake MAC failure";
            hs_payload.insert(hs_payload.end(), err.begin(), err.end());
        }
        out.push_back(0x01);
        detail::append_u16(out, hs_payload.size());
        out.insert(out.end(), hs_payload.begin(), hs_payload.end());
        ::send(fd, out.data(), out.size(), 0);
        return noise_ready_;
    }

    void serve() {
        const int fd = ::accept(listen_fd_, nullptr, nullptr);
        if (fd < 0) {
            return;
        }
        ByteBuffer buf;
        const bool noise = !noise_psk_b64.empty();
        if (noise && !noise_handshake(fd, buf)) {
            ::close(fd);
            return;
        }

        if (noise) {
            ByteBuffer frame;
            while (recv_noise_frame(fd, buf, frame)) {
                ByteBuffer pt;
                try {
                    pt = recv_cipher_.decrypt_with_ad(ByteView{},
                                                      ByteView(frame.data(), frame.size()));
                } catch (...) {
                    break;
                }
                const std::uint32_t type = (static_cast<std::uint32_t>(pt[0]) << 8) | pt[1];
                const std::size_t mlen = (static_cast<std::size_t>(pt[2]) << 8) | pt[3];
                if (const ByteView payload(pt.data() + 4, mlen); handle(fd, type, payload)) {
                    break;
                }
            }
        } else {
            PlaintextFrameHelper frames;
            std::array<std::uint8_t, 4096> tmp{};
            bool closing = false;
            while (!closing) {
                const ssize_t n = ::recv(fd, tmp.data(), tmp.size(), 0);
                if (n <= 0) {
                    break;
                }
                frames.feed(ByteView(tmp.data(), static_cast<std::size_t>(n)));
                std::uint32_t type = 0;
                ByteView payload;
                while (frames.next(type, payload) == FrameStatus::Ok) {
                    if (handle(fd, type, payload)) {
                        closing = true;
                        break;
                    }
                }
            }
        }
        ::close(fd);
    }

    // Returns true when the session should close.
    bool handle(int fd, std::uint32_t type, ByteView payload) {
        switch (static_cast<MessageId>(type)) {
        case MessageId::HelloRequest: {
            proto::HelloResponse resp;
            resp.set_api_version_major(1);
            resp.set_api_version_minor(10);
            resp.set_name(device_name);
            resp.set_server_info("loopback");
            send(fd, resp);
            break;
        }
        case MessageId::DeviceInfoRequest: {
            proto::DeviceInfoResponse resp;
            resp.set_name(device_name);
            resp.set_model("mock");
            resp.set_manufacturer("esphome-api-client tests");
            resp.set_esphome_version("0.0.0");
            resp.set_api_encryption_supported(!noise_psk_b64.empty());
            resp.add_serial_proxies()->set_name("console");    // instance 0
            resp.add_serial_proxies()->set_name("rs485-bus");  // instance 1
            send(fd, resp);
            break;
        }
        case MessageId::PingRequest: {
            proto::PingResponse resp;
            send(fd, resp);
            break;
        }
        case MessageId::SubscribeLogsRequest: {
            proto::SubscribeLogsResponse resp;
            resp.set_level(proto::LOG_LEVEL_INFO);
            resp.set_message("hello from loopback");
            send(fd, resp);
            break;
        }
        case MessageId::SubscribeBluetoothLEAdvertisementsRequest: {
            proto::BluetoothLEAdvertisementResponse resp;
            resp.set_address(0x112233445566ULL);
            resp.set_name("ble-device");
            resp.set_rssi(-42);
            send(fd, resp);
            break;
        }
        case MessageId::ListEntitiesRequest: {
            proto::ListEntitiesSwitchResponse sw;
            sw.set_key(10);
            sw.set_object_id("relay");
            sw.set_name("Relay");
            send(fd, sw);
            proto::ListEntitiesLightResponse lt;
            lt.set_key(11);
            lt.set_object_id("lamp");
            lt.set_name("Lamp");
            send(fd, lt);
            proto::ListEntitiesSensorResponse se;
            se.set_key(12);
            se.set_object_id("temperature");
            se.set_name("Temperature");
            se.set_unit_of_measurement("\xc2\xb0"
                                       "C");
            send(fd, se);
            proto::ListEntitiesDoneResponse done;
            send(fd, done);
            break;
        }
        case MessageId::SubscribeStatesRequest: {
            // Emit an initial sensor reading so state accessors have data.
            proto::SensorStateResponse st;
            st.set_key(12);
            st.set_state(21.5F);
            send(fd, st);
            break;
        }
        case MessageId::SwitchCommandRequest: {
            proto::SwitchCommandRequest req;
            static_cast<void>(req.ParseFromArray(payload.data(), static_cast<int>(payload.size())));
            proto::SwitchStateResponse st;
            st.set_key(req.key());
            st.set_state(req.state());
            send(fd, st);
            break;
        }
        case MessageId::SerialProxyGetModemPinsRequest: {
            proto::SerialProxyGetModemPinsRequest req;
            static_cast<void>(req.ParseFromArray(payload.data(), static_cast<int>(payload.size())));
            proto::SerialProxyGetModemPinsResponse resp;
            resp.set_instance(req.instance());
            resp.set_line_states(0x3);  // RTS | DTR
            send(fd, resp);
            break;
        }
        case MessageId::BluetoothGATTReadRequest: {
            proto::BluetoothGATTReadRequest req;
            static_cast<void>(req.ParseFromArray(payload.data(), static_cast<int>(payload.size())));
            proto::BluetoothGATTReadResponse resp;
            resp.set_address(req.address());
            resp.set_handle(req.handle());
            resp.set_data("gatt-value");
            send(fd, resp);
            break;
        }
        case MessageId::DisconnectRequest: {
            proto::DisconnectResponse resp;
            send(fd, resp);
            return true;
        }
        default:
            break;
        }
        return false;
    }

    int listen_fd_ = -1;
    std::uint16_t port_ = 0;
    std::thread thread_;

    bool noise_ready_ = false;
    noise::CipherState send_cipher_;
    noise::CipherState recv_cipher_;
};

}  // namespace esphome::api::testing
