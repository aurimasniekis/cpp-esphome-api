#include <esphome/api/bytes.hpp>

namespace esphome::api {

std::size_t varint_size(std::uint32_t value) noexcept {
    std::size_t n = 1;
    while (value >= 0x80) {
        value >>= 7;
        ++n;
    }
    return n;
}

void append_varint(ByteBuffer& out, std::uint32_t value) {
    while (value >= 0x80) {
        out.push_back(static_cast<std::uint8_t>(value) | 0x80U);
        value >>= 7;
    }
    out.push_back(static_cast<std::uint8_t>(value));
}

VarintStatus decode_varint(const std::uint8_t* data,
                           const std::size_t len,
                           std::uint32_t& value,
                           std::size_t& consumed,
                           const std::size_t max_bytes) noexcept {
    std::uint64_t result = 0;
    std::size_t shift = 0;
    for (std::size_t i = 0; i < len; ++i) {
        if (i >= max_bytes) {
            return VarintStatus::Overflow;
        }
        const std::uint8_t byte = data[i];
        result |= static_cast<std::uint64_t>(byte & 0x7FU) << shift;
        if ((byte & 0x80U) == 0) {
            if (result > 0xFFFFFFFFULL) {
                return VarintStatus::Overflow;
            }
            value = static_cast<std::uint32_t>(result);
            consumed = i + 1;
            return VarintStatus::Ok;
        }
        shift += 7;
    }
    // Ran out of input while the high continuation bit was still set: either a
    // valid prefix (Incomplete) or, if we already spent the whole budget on
    // continuation bytes, an overflow.
    if (len >= max_bytes) {
        return VarintStatus::Overflow;
    }
    return VarintStatus::Incomplete;
}

}  // namespace esphome::api
