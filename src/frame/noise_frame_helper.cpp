#include <esphome/api/exception.hpp>
#include <esphome/api/frame/noise_frame_helper.hpp>

#include <array>
#include <utility>

namespace esphome::api {

namespace {

// 14-byte prologue mixed into the handshake transcript.
constexpr std::array<std::uint8_t, 14> noise_prologue = {
    'N', 'o', 'i', 's', 'e', 'A', 'P', 'I', 'I', 'n', 'i', 't', 0x00, 0x00};

ByteView prologue_view() {
    return ByteView(noise_prologue.data(), noise_prologue.size());
}

void append_u16(ByteBuffer& out, const std::size_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

}  // namespace

NoiseFrameHelper::NoiseFrameHelper(const NoiseConfig& config)
    : expected_name_(config.expected_name) {
    ByteBuffer decoded;
    if (!noise::base64_decode(config.psk_base64, decoded) || decoded.size() != noise::key_len) {
        throw ApiError("invalid Noise PSK: expected base64 of a 32-byte key");
    }
    std::copy(decoded.begin(), decoded.end(), psk_.begin());
    handshake_.initialize(noise::NoiseHandshake::Role::Initiator, psk_, prologue_view());
}

ByteBuffer NoiseFrameHelper::connect_bytes() {
    // First flight: NOISE_HELLO (empty frame) + framed [0x00 + write_message(e)].
    const ByteBuffer handshake_msg = handshake_.write_message();

    ByteBuffer out;
    // NOISE_HELLO = 0x01 0x00 0x00 (an empty frame announcing the protocol).
    out.push_back(indicator);
    out.push_back(0x00);
    out.push_back(0x00);

    // Handshake frame: indicator, u16 len, payload = 0x00 (success marker) + msg.
    const std::size_t payload_len = handshake_msg.size() + 1;
    out.push_back(indicator);
    append_u16(out, payload_len);
    out.push_back(0x00);
    out.insert(out.end(), handshake_msg.begin(), handshake_msg.end());
    return out;
}

void NoiseFrameHelper::set_handshake_handlers(HandshakeReady on_ready, HandshakeError on_error) {
    on_ready_ = std::move(on_ready);
    on_error_ = std::move(on_error);
}

void NoiseFrameHelper::encode(const std::uint32_t msg_type,
                              const ByteView payload,
                              ByteBuffer& out) {
    // Inner plaintext: [u16 type][u16 len][payload].
    ByteBuffer inner;
    append_u16(inner, static_cast<std::size_t>(msg_type));
    append_u16(inner, payload.size());
    inner.insert(inner.end(), payload.begin(), payload.end());

    const ByteBuffer ct = encrypt_.encrypt_with_ad(ByteView{}, inner);
    out.push_back(indicator);
    append_u16(out, ct.size());
    out.insert(out.end(), ct.begin(), ct.end());
}

void NoiseFrameHelper::feed(const ByteView data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

void NoiseFrameHelper::compact() {
    if (consumed_ == 0) {
        return;
    }
    if (consumed_ >= buffer_.size()) {
        buffer_.clear();
    } else {
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(consumed_));
    }
    consumed_ = 0;
}

FrameStatus NoiseFrameHelper::next_raw(ByteView& out_payload) {
    compact();
    const std::uint8_t* p = buffer_.data();
    const std::size_t n = buffer_.size();
    if (n < 3) {
        return FrameStatus::NeedMore;
    }
    if (p[0] != indicator) {
        if (p[0] == 0x00) {  // 0x00 is the plaintext frame indicator
            throw EncryptionMismatchError(
                "device is not using encryption; retry without --key / -k");
        }
        throw ProtocolError("noise frame: bad indicator byte (expected 0x01)");
    }
    const std::size_t len = (static_cast<std::size_t>(p[1]) << 8) | p[2];
    if (len > max_frame_payload) {
        throw ProtocolError("noise frame: length exceeds maximum");
    }
    if (n - 3 < len) {
        return FrameStatus::NeedMore;
    }
    out_payload = ByteView(p + 3, len);
    consumed_ = 3 + len;
    return FrameStatus::Ok;
}

FrameStatus NoiseFrameHelper::next(std::uint32_t& out_type, ByteView& out_payload) {
    for (;;) {
        ByteView raw;
        if (const FrameStatus st = next_raw(raw); st == FrameStatus::NeedMore) {
            return FrameStatus::NeedMore;
        }

        switch (state_) {
        case State::ExpectServerHello:
            handle_server_hello(raw);
            break;
        case State::ExpectHandshake:
            handle_server_handshake(raw);
            break;
        case State::Failed:
            return FrameStatus::NeedMore;
        case State::Ready: {
            ByteBuffer pt;
            try {
                pt = decrypt_.decrypt_with_ad(ByteView{}, raw);
            } catch (const EncryptionError& e) {
                throw ProtocolError(std::string("noise frame decrypt failed: ") + e.what());
            }
            if (pt.size() < 4) {
                throw ProtocolError("noise frame: inner header too short");
            }
            const std::uint32_t type =
                (static_cast<std::uint32_t>(pt[0]) << 8) | static_cast<std::uint32_t>(pt[1]);
            const std::size_t msg_len = (static_cast<std::size_t>(pt[2]) << 8) | pt[3];
            if (pt.size() < 4 + msg_len) {
                throw ProtocolError("noise frame: inner length exceeds plaintext");
            }
            plaintext_ = std::move(pt);
            out_type = type;
            out_payload = ByteView(plaintext_.data() + 4, msg_len);
            return FrameStatus::Ok;
        }
        }
    }
}

void NoiseFrameHelper::handle_server_hello(const ByteView payload) {
    if (payload.empty()) {
        report_error("empty server hello");
        return;
    }
    if (const std::uint8_t chosen_proto = payload[0]; chosen_proto != 0x01) {
        report_error("device selected an unsupported encryption protocol");
        return;
    }
    // Server name runs from index 1 to the first NUL.
    std::size_t end = 1;
    while (end < payload.size() && payload[end] != 0x00) {
        ++end;
    }
    server_name_.assign(reinterpret_cast<const char*>(payload.data() + 1), end - 1);
    if (!expected_name_.empty() && server_name_ != expected_name_) {
        report_error("connected to '" + server_name_ + "' but expected '" + expected_name_ + "'");
        return;
    }
    state_ = State::ExpectHandshake;
}

void NoiseFrameHelper::handle_server_handshake(const ByteView payload) {
    if (payload.empty()) {
        report_error("empty handshake message");
        return;
    }
    if (payload[0] != 0x00) {
        // Error frame: the rest is a UTF-8 explanation (e.g. bad PSK).
        const std::string what(reinterpret_cast<const char*>(payload.data() + 1),
                               payload.size() - 1);
        report_error("handshake rejected by device: " + what);
        return;
    }
    if (ByteBuffer hs_payload; !handshake_.read_message(payload.subview(1), hs_payload)) {
        report_error("Noise handshake failed (wrong PSK?)");
        return;
    }
    handshake_.split(encrypt_, decrypt_);
    state_ = State::Ready;
    if (on_ready_) {
        on_ready_();
    }
}

void NoiseFrameHelper::report_error(const std::string& what) {
    state_ = State::Failed;
    if (on_error_) {
        on_error_(what);
    }
}

}  // namespace esphome::api
