// End-to-end test of NoiseFrameHelper against an in-test Noise responder: the
// full handshake handshake flight, then encrypted application-message exchange.

#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/crypto/noise_handshake.hpp>
#include <esphome/api/crypto/primitives.hpp>
#include <esphome/api/exception.hpp>
#include <esphome/api/frame/noise_frame_helper.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace noise = esphome::api::noise;
using esphome::api::ByteBuffer;
using esphome::api::ByteView;
using esphome::api::FrameStatus;
using esphome::api::NoiseConfig;
using esphome::api::NoiseFrameHelper;

namespace {

constexpr auto psk_b64 = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=";
constexpr auto wrong_psk_b64 = "AQQHCg0QExYZHB8iJSgrLjE0Nzo9QENGSUxPUlVYW14=";

ByteView view(const ByteBuffer& b) {
    return ByteView(b.data(), b.size());
}

void append_u16(ByteBuffer& out, const std::size_t v) {
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

ByteBuffer frame_of(const ByteView payload) {
    ByteBuffer out;
    out.push_back(0x01);
    append_u16(out, payload.size());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

// Minimal Noise responder mirroring the device side, for driving the client
// NoiseFrameHelper through a full session.
class TestResponder {
public:
    explicit TestResponder(const char* psk_base64, std::string name = "device")
        : name_(std::move(name)) {
        ByteBuffer decoded;
        noise::base64_decode(psk_base64, decoded);
        for (std::size_t i = 0; i < psk_.size(); ++i) {
            psk_[i] = decoded[i];
        }
        static const ByteBuffer prologue = {
            'N', 'o', 'i', 's', 'e', 'A', 'P', 'I', 'I', 'n', 'i', 't', 0, 0};
        handshake_.initialize(noise::NoiseHandshake::Role::Responder, psk_, view(prologue));
    }

    // Consume the client's first flight; return the server's response bytes.
    ByteBuffer respond_to_first_flight(const ByteBuffer& client_bytes) {
        // Parse frames: NOISE_HELLO (empty) then handshake frame.
        std::size_t pos = 0;
        ByteBuffer e_msg;
        while (pos + 3 <= client_bytes.size()) {
            const std::size_t len =
                (static_cast<std::size_t>(client_bytes[pos + 1]) << 8) | client_bytes[pos + 2];
            const std::size_t start = pos + 3;
            if (len > 0) {  // the handshake frame; payload[0] is the success marker
                e_msg.assign(client_bytes.begin() + static_cast<std::ptrdiff_t>(start + 1),
                             client_bytes.begin() + static_cast<std::ptrdiff_t>(start + len));
            }
            pos = start + len;
        }

        // Server hello: [proto=0x01][name\0].
        ByteBuffer hello_payload;
        hello_payload.push_back(0x01);
        hello_payload.insert(hello_payload.end(), name_.begin(), name_.end());
        hello_payload.push_back(0x00);

        ByteBuffer out = frame_of(view(hello_payload));

        ByteBuffer hs_payload;
        if (ByteBuffer got; handshake_.read_message(view(e_msg), got)) {
            hs_payload.push_back(0x00);
            const ByteBuffer reply = handshake_.write_message();
            hs_payload.insert(hs_payload.end(), reply.begin(), reply.end());
            handshake_.split(send_, recv_);
            ready_ = true;
        } else {
            hs_payload.push_back(0x01);
            const std::string err = "Handshake MAC failure";
            hs_payload.insert(hs_payload.end(), err.begin(), err.end());
        }
        const ByteBuffer hs_frame = frame_of(view(hs_payload));
        out.insert(out.end(), hs_frame.begin(), hs_frame.end());
        return out;
    }

    [[nodiscard]] bool ready() const {
        return ready_;
    }

    // Encrypt one application message into a frame.
    ByteBuffer encrypt(const std::uint32_t type, const ByteView payload) {
        ByteBuffer inner;
        append_u16(inner, type);
        append_u16(inner, payload.size());
        inner.insert(inner.end(), payload.begin(), payload.end());
        const ByteBuffer ct = send_.encrypt_with_ad(ByteView{}, view(inner));
        return frame_of(view(ct));
    }

    // Decrypt one application frame; returns inner (type,payload).
    bool decrypt(const ByteBuffer& wire, std::uint32_t& type, ByteBuffer& payload) {
        const std::size_t len = (static_cast<std::size_t>(wire[1]) << 8) | wire[2];
        const ByteView ct(wire.data() + 3, len);
        ByteBuffer pt;
        try {
            pt = recv_.decrypt_with_ad(ByteView{}, ct);
        } catch (...) {
            return false;
        }
        type = (static_cast<std::uint32_t>(pt[0]) << 8) | pt[1];
        const std::size_t mlen = (static_cast<std::size_t>(pt[2]) << 8) | pt[3];
        payload.assign(pt.begin() + 4, pt.begin() + 4 + static_cast<std::ptrdiff_t>(mlen));
        return true;
    }

private:
    std::string name_;
    noise::SymmetricKey psk_{};
    noise::NoiseHandshake handshake_;
    noise::CipherState send_;
    noise::CipherState recv_;
    bool ready_ = false;
};

}  // namespace

TEST(NoiseFrame, FullHandshakeAndExchange) {
    NoiseConfig config;
    config.psk_base64 = psk_b64;
    NoiseFrameHelper client(config);

    bool ready = false;
    std::string error;
    client.set_handshake_handlers([&] { ready = true; }, [&](const std::string& e) { error = e; });

    TestResponder server(psk_b64, "device");

    // Client first flight -> server -> client.
    const ByteBuffer first = client.connect_bytes();
    const ByteBuffer server_response = server.respond_to_first_flight(first);
    ASSERT_TRUE(server.ready());

    client.feed(server_response);
    std::uint32_t type = 0;
    ByteView payload;
    EXPECT_EQ(client.next(type, payload), FrameStatus::NeedMore);  // handshake consumed
    EXPECT_TRUE(ready);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(client.server_name(), "device");

    // Client -> server application message.
    const ByteBuffer hello = {0xDE, 0xAD, 0xBE, 0xEF};
    ByteBuffer wire;
    client.encode(42, view(hello), wire);
    std::uint32_t got_type = 0;
    ByteBuffer got_payload;
    ASSERT_TRUE(server.decrypt(wire, got_type, got_payload));
    EXPECT_EQ(got_type, 42U);
    EXPECT_EQ(got_payload, hello);

    // Server -> client application message.
    const ByteBuffer state = {0x01, 0x02, 0x03};
    const ByteBuffer server_frame = server.encrypt(99, view(state));
    client.feed(server_frame);
    ASSERT_EQ(client.next(type, payload), FrameStatus::Ok);
    EXPECT_EQ(type, 99U);
    EXPECT_EQ(ByteBuffer(payload.begin(), payload.end()), state);
}

TEST(NoiseFrame, WrongPskFailsHandshake) {
    NoiseConfig config;
    config.psk_base64 = wrong_psk_b64;  // client has the wrong key
    NoiseFrameHelper client(config);

    bool ready = false;
    std::string error;
    client.set_handshake_handlers([&] { ready = true; }, [&](const std::string& e) { error = e; });

    TestResponder server(psk_b64, "device");  // server has the right key

    const ByteBuffer first = client.connect_bytes();
    const ByteBuffer server_response = server.respond_to_first_flight(first);
    EXPECT_FALSE(server.ready());  // server rejected the handshake

    client.feed(server_response);
    std::uint32_t type = 0;
    ByteView payload;
    client.next(type, payload);
    EXPECT_FALSE(ready);
    EXPECT_FALSE(error.empty());
}

TEST(NoiseFrame, PlaintextIndicatorReportsEncryptionUnexpected) {
    NoiseConfig config;
    config.psk_base64 = psk_b64;
    NoiseFrameHelper client(config);
    client.set_handshake_handlers([] {}, [](const std::string&) {});

    ByteBuffer bad = {0x00, 0x00, 0x01, 0xFF};  // plaintext indicator on a noise helper
    client.feed(bad);
    std::uint32_t type = 0;
    ByteView payload;
    // A plaintext-only device reached with a PSK is a specific, actionable case.
    EXPECT_THROW(client.next(type, payload), esphome::api::EncryptionMismatchError);
    try {
        client.next(type, payload);
        FAIL() << "expected EncryptionMismatchError";
    } catch (const esphome::api::EncryptionMismatchError& e) {
        EXPECT_NE(std::string(e.what()).find("without --key"), std::string::npos);
    }
}

TEST(NoiseFrame, OtherBadIndicatorThrowsProtocolError) {
    NoiseConfig config;
    config.psk_base64 = psk_b64;
    NoiseFrameHelper client(config);
    client.set_handshake_handlers([] {}, [](const std::string&) {});

    ByteBuffer bad = {0x42, 0x00, 0x01, 0xFF};  // neither Noise (0x01) nor plaintext (0x00)
    client.feed(bad);
    std::uint32_t type = 0;
    ByteView payload;
    EXPECT_THROW(client.next(type, payload), esphome::api::ProtocolError);
}

TEST(NoiseFrame, InvalidPskRejectedAtConstruction) {
    NoiseConfig config;
    config.psk_base64 = "not-valid-base64-or-wrong-length";
    EXPECT_THROW(NoiseFrameHelper{config}, esphome::api::ApiError);
}
