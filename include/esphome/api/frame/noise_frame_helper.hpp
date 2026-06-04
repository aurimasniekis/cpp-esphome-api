#pragma once

/// @file
/// @brief Noise-encrypted ESPHome frame codec (Noise_NNpsk0_25519_ChaChaPoly_SHA256).

#include <esphome/api/bytes.hpp>
#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/crypto/noise_handshake.hpp>
#include <esphome/api/crypto/primitives.hpp>
#include <esphome/api/frame/frame_helper.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome::api {

/// Configuration for an encrypted connection.
struct NoiseConfig {
    /// Base64-encoded 32-byte pre-shared key (the device's `api.encryption.key`).
    std::string psk_base64;
    /// Optional expected device name, verified against the server hello.
    std::string expected_name;
};

/// Encrypted frame codec. Wire frames are `[0x01][u16_be len][ciphertext]`. It
/// drives the NNpsk0 handshake internally (server-hello then server-handshake),
/// then encrypts/decrypts application messages whose inner plaintext is
/// `[u16_be type][u16_be len][protobuf payload]`.
class NoiseFrameHelper final : public FrameHelper {
public:
    /// Indicator byte prefixing every Noise frame.
    static constexpr std::uint8_t indicator = 0x01;

    /// Construct from config. Throws ApiError if the PSK is not valid base64 of
    /// exactly 32 bytes.
    explicit NoiseFrameHelper(const NoiseConfig& config);

    void encode(std::uint32_t msg_type, ByteView payload, ByteBuffer& out) override;
    void feed(ByteView data) override;
    FrameStatus next(std::uint32_t& out_type, ByteView& out_payload) override;

    [[nodiscard]] bool needs_handshake() const override {
        return true;
    }
    ByteBuffer connect_bytes() override;
    void set_handshake_handlers(HandshakeReady on_ready, HandshakeError on_error) override;

    [[nodiscard]] const std::string& server_name() const noexcept {
        return server_name_;
    }

private:
    enum class State { ExpectServerHello, ExpectHandshake, Ready, Failed };

    FrameStatus next_raw(ByteView& out_payload);
    void compact();
    void handle_server_hello(ByteView payload);
    void handle_server_handshake(ByteView payload);
    void report_error(const std::string& what);

    noise::SymmetricKey psk_{};
    std::string expected_name_;
    std::string server_name_;

    noise::NoiseHandshake handshake_;
    noise::CipherState encrypt_;
    noise::CipherState decrypt_;
    State state_ = State::ExpectServerHello;

    HandshakeReady on_ready_;
    HandshakeError on_error_;

    ByteBuffer buffer_;
    std::size_t consumed_ = 0;
    ByteBuffer plaintext_;
};

}  // namespace esphome::api
