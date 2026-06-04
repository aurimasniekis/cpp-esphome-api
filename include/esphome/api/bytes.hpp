#pragma once

/// @file
/// @brief Byte buffer / view aliases and protobuf base-128 varint codec.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace esphome::api {

/// Owning, growable byte buffer.
using ByteBuffer = std::vector<std::uint8_t>;

/// Non-owning view over a contiguous run of bytes (a C++17 stand-in for
/// std::span<const std::uint8_t>). The viewed storage must outlive the view.
class ByteView {
public:
    constexpr ByteView() noexcept = default;
    constexpr ByteView(const std::uint8_t* data, const std::size_t size) noexcept
        : data_(data), size_(size) {}

    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    ByteView(const ByteBuffer& buf) noexcept : data_(buf.data()), size_(buf.size()) {}

    [[nodiscard]] constexpr const std::uint8_t* data() const noexcept {
        return data_;
    }
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return size_;
    }
    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] constexpr const std::uint8_t* begin() const noexcept {
        return data_;
    }
    [[nodiscard]] constexpr const std::uint8_t* end() const noexcept {
        return data_ + size_;
    }

    [[nodiscard]] constexpr std::uint8_t operator[](const std::size_t i) const noexcept {
        return data_[i];
    }

    /// Sub-view starting at `offset` (clamped) spanning the remaining bytes.
    [[nodiscard]] ByteView subview(const std::size_t offset) const noexcept {
        if (offset >= size_) {
            return {};
        }
        return {data_ + offset, size_ - offset};
    }

private:
    const std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
};

/// Maximum number of bytes a base-128 varint may occupy for a 32-bit value.
inline constexpr std::size_t max_varint_bytes = 5;

/// Outcome of decoding a varint from a (possibly partial) buffer.
enum class VarintStatus {
    Ok,          ///< A complete value was decoded.
    Incomplete,  ///< The buffer ended mid-varint; read more and retry.
    Overflow,    ///< The varint exceeded the permitted byte budget.
};

/// Number of bytes the base-128 encoding of `value` occupies.
[[nodiscard]] std::size_t varint_size(std::uint32_t value) noexcept;

/// Append the base-128 varint encoding of `value` to `out`.
void append_varint(ByteBuffer& out, std::uint32_t value);

/// Decode a base-128 varint from `[data, data+len)`.
///
/// On VarintStatus::Ok, `value` holds the decoded number and `consumed` the
/// number of bytes read. On ::Incomplete the buffer is a valid varint prefix.
/// On ::Overflow more than `max_bytes` continuation bytes were seen.
[[nodiscard]] VarintStatus decode_varint(const std::uint8_t* data,
                                         std::size_t len,
                                         std::uint32_t& value,
                                         std::size_t& consumed,
                                         std::size_t max_bytes = max_varint_bytes) noexcept;

}  // namespace esphome::api
