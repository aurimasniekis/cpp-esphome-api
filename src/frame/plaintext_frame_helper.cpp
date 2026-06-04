#include <esphome/api/exception.hpp>
#include <esphome/api/frame/plaintext_frame_helper.hpp>

#include <cstring>

namespace esphome::api {

namespace {
// Length and type varints in the plaintext framing are capped at 4 bytes, per
// the ESPHome wire protocol.
constexpr std::size_t varint_budget = 4;
}  // namespace

void PlaintextFrameHelper::encode(const std::uint32_t msg_type,
                                  const ByteView payload,
                                  ByteBuffer& out) {
    out.push_back(indicator);
    append_varint(out, static_cast<std::uint32_t>(payload.size()));
    append_varint(out, msg_type);
    out.insert(out.end(), payload.begin(), payload.end());
}

void PlaintextFrameHelper::feed(const ByteView data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

void PlaintextFrameHelper::compact() {
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

FrameStatus PlaintextFrameHelper::next(std::uint32_t& out_type, ByteView& out_payload) {
    compact();

    const std::uint8_t* p = buffer_.data();
    const std::size_t n = buffer_.size();
    if (n < 1) {
        return FrameStatus::NeedMore;
    }
    if (p[0] != indicator) {
        if (p[0] == 0x01) {  // 0x01 is the Noise frame indicator
            throw EncryptionMismatchError(
                "device requires encryption; provide a Noise PSK with --key / -k");
        }
        throw ProtocolError("plaintext frame: bad indicator byte (got 0x" +
                            std::to_string(static_cast<int>(p[0])) + ", expected 0x00)");
    }

    std::size_t cursor = 1;

    std::uint32_t payload_len = 0;
    std::size_t consumed = 0;
    switch (decode_varint(p + cursor, n - cursor, payload_len, consumed, varint_budget)) {
    case VarintStatus::Incomplete:
        return FrameStatus::NeedMore;
    case VarintStatus::Overflow:
        throw ProtocolError("plaintext frame: payload length varint too long");
    case VarintStatus::Ok:
        break;
    }
    cursor += consumed;

    std::uint32_t msg_type = 0;
    switch (decode_varint(p + cursor, n - cursor, msg_type, consumed, varint_budget)) {
    case VarintStatus::Incomplete:
        return FrameStatus::NeedMore;
    case VarintStatus::Overflow:
        throw ProtocolError("plaintext frame: message type varint too long");
    case VarintStatus::Ok:
        break;
    }
    cursor += consumed;

    if (payload_len > max_frame_payload) {
        throw ProtocolError("plaintext frame: payload length exceeds maximum");
    }
    if (n - cursor < payload_len) {
        return FrameStatus::NeedMore;
    }

    out_type = msg_type;
    out_payload = ByteView(p + cursor, payload_len);
    consumed_ = cursor + payload_len;
    return FrameStatus::Ok;
}

}  // namespace esphome::api
