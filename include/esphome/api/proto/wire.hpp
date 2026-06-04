#pragma once

/// @file
/// @brief Self-contained proto3 wire-format primitives (varint / zigzag /
///        fixed32 / fixed64 / length-delimited), with a writer and a reader.
///
/// This replaces Google protobuf's runtime. The encoding produced here is
/// byte-for-byte identical to `protoc`-generated C++ for the proto3 subset the
/// ESPHome native API uses (scalars, string, bytes, enums, repeated, packed,
/// and flat sub-messages). The contract is enforced by the golden test.

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace esphome::api {

/// Protobuf wire types (the low 3 bits of a field tag).
enum class WireType : std::uint8_t {
    Varint = 0,           ///< int32/int64/uint32/uint64/sint32/sint64/bool/enum
    Fixed64 = 1,          ///< fixed64/sfixed64/double
    LengthDelimited = 2,  ///< string/bytes/embedded message/packed repeated
    Fixed32 = 5,          ///< fixed32/sfixed32/float
};

// ---------------------------------------------------------------------------
// ZigZag encoding for sint32 / sint64.
// ---------------------------------------------------------------------------

inline std::uint32_t zigzag_encode32(const std::int32_t v) {
    return (static_cast<std::uint32_t>(v) << 1) ^ static_cast<std::uint32_t>(v >> 31);
}
inline std::int32_t zigzag_decode32(const std::uint32_t v) {
    return static_cast<std::int32_t>((v >> 1) ^ (~(v & 1) + 1));
}
inline std::uint64_t zigzag_encode64(const std::int64_t v) {
    return (static_cast<std::uint64_t>(v) << 1) ^ static_cast<std::uint64_t>(v >> 63);
}
inline std::int64_t zigzag_decode64(const std::uint64_t v) {
    return static_cast<std::int64_t>((v >> 1) ^ (~(v & 1) + 1));
}

// ---------------------------------------------------------------------------
// Size helpers (mirror the encoder so calculate_size() can be exact).
// ---------------------------------------------------------------------------

/// Number of bytes a value occupies as a base-128 varint.
inline std::size_t varint_size(std::uint64_t v) {
    std::size_t n = 1;
    while (v >= 0x80) {
        v >>= 7;
        ++n;
    }
    return n;
}

/// Number of bytes the tag for `field_number` occupies (wire type does not
/// change the byte count — it only fills the low 3 bits).
inline std::size_t tag_size(const std::uint32_t field_number) {
    return varint_size(static_cast<std::uint64_t>(field_number) << 3);
}

// ---------------------------------------------------------------------------
// Writer — appends to a std::string buffer.
// ---------------------------------------------------------------------------

class ProtoWriter {
public:
    explicit ProtoWriter(std::string& out) : out_(out) {}

    void raw_byte(const std::uint8_t b) const {
        out_.push_back(static_cast<char>(b));
    }
    void raw_bytes(const void* data, const std::size_t n) const {
        out_.append(static_cast<const char*>(data), n);
    }

    void varint(std::uint64_t v) const {
        while (v >= 0x80) {
            raw_byte(static_cast<std::uint8_t>(v) | 0x80U);
            v >>= 7;
        }
        raw_byte(static_cast<std::uint8_t>(v));
    }

    void tag(const std::uint32_t field_number, WireType wt) const {
        varint((static_cast<std::uint64_t>(field_number) << 3) | static_cast<std::uint64_t>(wt));
    }

    void fixed32(const std::uint32_t v) const {
        std::array<std::uint8_t, 4> b{};
        for (std::size_t i = 0; i < b.size(); ++i)
            b[i] = static_cast<std::uint8_t>(v >> (8 * i));
        raw_bytes(b.data(), b.size());
    }

    void fixed64(const std::uint64_t v) const {
        std::array<std::uint8_t, 8> b{};
        for (std::size_t i = 0; i < b.size(); ++i)
            b[i] = static_cast<std::uint8_t>(v >> (8 * i));
        raw_bytes(b.data(), b.size());
    }

    void length_delimited(const std::string& s) const {
        varint(s.size());
        raw_bytes(s.data(), s.size());
    }

private:
    std::string& out_;
};

// ---------------------------------------------------------------------------
// Reader — consumes a byte range. Methods return false on truncation / error
// and latch an error flag so the decode loop can bail out.
// ---------------------------------------------------------------------------

class ProtoReader {
public:
    ProtoReader() = default;
    ProtoReader(const void* data, const std::size_t size)
        : ptr_(static_cast<const std::uint8_t*>(data)),
          end_(static_cast<const std::uint8_t*>(data) + size) {}

    [[nodiscard]] bool ok() const {
        return ok_;
    }
    [[nodiscard]] bool eof() const {
        return ptr_ >= end_;
    }

    /// Read a field tag. Returns false at clean end-of-input or on error;
    /// distinguish with ok().
    bool read_tag(std::uint32_t& field_number, WireType& wt) {
        if (ptr_ >= end_)
            return false;
        std::uint64_t key = 0;
        if (!varint(key))
            return false;
        field_number = static_cast<std::uint32_t>(key >> 3);
        wt = static_cast<WireType>(key & 0x7U);
        return true;
    }

    bool varint(std::uint64_t& out) {
        std::uint64_t result = 0;
        int shift = 0;
        while (shift < 64) {
            if (ptr_ >= end_)
                return fail();
            const std::uint8_t b = *ptr_++;
            result |= static_cast<std::uint64_t>(b & 0x7FU) << shift;
            if ((b & 0x80U) == 0) {
                out = result;
                return true;
            }
            shift += 7;
        }
        return fail();
    }

    bool fixed32(std::uint32_t& out) {
        if (end_ - ptr_ < 4)
            return fail();
        std::uint32_t v = 0;
        v |= static_cast<std::uint32_t>(ptr_[0]);
        v |= static_cast<std::uint32_t>(ptr_[1]) << 8;
        v |= static_cast<std::uint32_t>(ptr_[2]) << 16;
        v |= static_cast<std::uint32_t>(ptr_[3]) << 24;
        ptr_ += 4;
        out = v;
        return true;
    }

    bool fixed64(std::uint64_t& out) {
        if (end_ - ptr_ < 8)
            return fail();
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i)
            v |= static_cast<std::uint64_t>(ptr_[i]) << (8 * i);
        ptr_ += 8;
        out = v;
        return true;
    }

    /// Read a length-delimited blob into `out`.
    bool length_delimited(std::string& out) {
        std::uint64_t len = 0;
        if (!varint(len))
            return false;
        if (static_cast<std::uint64_t>(end_ - ptr_) < len)
            return fail();
        out.assign(reinterpret_cast<const char*>(ptr_), static_cast<std::size_t>(len));
        ptr_ += len;
        return true;
    }

    /// Obtain a sub-reader over the next length-delimited region (for embedded
    /// messages and packed repeated fields) and advance past it.
    bool sub_reader(ProtoReader& out) {
        std::uint64_t len = 0;
        if (!varint(len))
            return false;
        if (static_cast<std::uint64_t>(end_ - ptr_) < len)
            return fail();
        out = ProtoReader(ptr_, static_cast<std::size_t>(len));
        ptr_ += len;
        return true;
    }

    /// Skip a field of the given wire type (unknown field forward-compat).
    bool skip_field(const WireType wt) {
        switch (wt) {
        case WireType::Varint: {
            std::uint64_t tmp = 0;
            return varint(tmp);
        }
        case WireType::Fixed64: {
            std::uint64_t tmp = 0;
            return fixed64(tmp);
        }
        case WireType::Fixed32: {
            std::uint32_t tmp = 0;
            return fixed32(tmp);
        }
        case WireType::LengthDelimited: {
            std::uint64_t len = 0;
            if (!varint(len))
                return false;
            if (static_cast<std::uint64_t>(end_ - ptr_) < len)
                return fail();
            ptr_ += len;
            return true;
        }
        }
        return fail();
    }

private:
    bool fail() {
        ok_ = false;
        return false;
    }

    const std::uint8_t* ptr_ = nullptr;
    const std::uint8_t* end_ = nullptr;
    bool ok_ = true;
};

// ---------------------------------------------------------------------------
// float / double <-> integer bit reinterpretation (no bit_cast in C++17).
// ---------------------------------------------------------------------------

inline std::uint32_t float_to_bits(const float f) {
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}
inline float bits_to_float(const std::uint32_t bits) {
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}
inline std::uint64_t double_to_bits(const double d) {
    std::uint64_t bits;
    std::memcpy(&bits, &d, sizeof(bits));
    return bits;
}
inline double bits_to_double(const std::uint64_t bits) {
    double d;
    std::memcpy(&d, &bits, sizeof(d));
    return d;
}

}  // namespace esphome::api
