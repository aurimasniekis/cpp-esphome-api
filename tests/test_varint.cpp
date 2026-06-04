#include <esphome/api/bytes.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <vector>

using esphome::api::append_varint;
using esphome::api::ByteBuffer;
using esphome::api::decode_varint;
using esphome::api::varint_size;
using esphome::api::VarintStatus;

namespace {

std::uint32_t round_trip(const std::uint32_t value) {
    ByteBuffer buf;
    append_varint(buf, value);
    EXPECT_EQ(buf.size(), varint_size(value));
    std::uint32_t out = 0;
    std::size_t consumed = 0;
    EXPECT_EQ(decode_varint(buf.data(), buf.size(), out, consumed), VarintStatus::Ok);
    EXPECT_EQ(consumed, buf.size());
    return out;
}

}  // namespace

TEST(Varint, RoundTripBoundaries) {
    for (std::uint32_t v : {0U,
                            1U,
                            127U,
                            128U,
                            255U,
                            16383U,
                            16384U,
                            2097151U,
                            2097152U,
                            268435455U,
                            268435456U,
                            0xFFFFFFFFU}) {
        EXPECT_EQ(round_trip(v), v) << "value " << v;
    }
}

TEST(Varint, EncodingLengths) {
    EXPECT_EQ(varint_size(0), 1U);
    EXPECT_EQ(varint_size(127), 1U);
    EXPECT_EQ(varint_size(128), 2U);
    EXPECT_EQ(varint_size(16383), 2U);
    EXPECT_EQ(varint_size(16384), 3U);
    EXPECT_EQ(varint_size(0xFFFFFFFFU), 5U);
}

TEST(Varint, KnownEncoding) {
    ByteBuffer buf;
    append_varint(buf, 300);  // 0xAC 0x02
    ASSERT_EQ(buf.size(), 2U);
    EXPECT_EQ(buf[0], 0xACU);
    EXPECT_EQ(buf[1], 0x02U);
}

TEST(Varint, IncompletePrefix) {
    // 0x80 alone is a valid varint prefix but not a complete value.
    constexpr std::array<std::uint8_t, 1> data = {0x80};
    std::uint32_t out = 0;
    std::size_t consumed = 0;
    EXPECT_EQ(decode_varint(data.data(), data.size(), out, consumed), VarintStatus::Incomplete);
}

TEST(Varint, PartialThenComplete) {
    ByteBuffer full;
    append_varint(full, 200000);  // 3 bytes
    ASSERT_EQ(full.size(), 3U);

    std::uint32_t out = 0;
    std::size_t consumed = 0;
    // Feed one byte short: still incomplete.
    EXPECT_EQ(decode_varint(full.data(), full.size() - 1, out, consumed), VarintStatus::Incomplete);
    // Feed everything: completes.
    EXPECT_EQ(decode_varint(full.data(), full.size(), out, consumed), VarintStatus::Ok);
    EXPECT_EQ(out, 200000U);
    EXPECT_EQ(consumed, 3U);
}

TEST(Varint, OverflowBeyondBudget) {
    // Five continuation bytes with the high bit always set: never terminates
    // within the 4-byte budget the framing layer allows for length/type.
    const std::array<std::uint8_t, 5> data = {0x80, 0x80, 0x80, 0x80, 0x80};
    std::uint32_t out = 0;
    std::size_t consumed = 0;
    EXPECT_EQ(decode_varint(data.data(), data.size(), out, consumed, 4), VarintStatus::Overflow);
}

TEST(Varint, OverflowTooLargeForU32) {
    // Six-byte varint encodes a value beyond 2^32.
    const std::array<std::uint8_t, 6> data = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01};
    std::uint32_t out = 0;
    std::size_t consumed = 0;
    EXPECT_EQ(decode_varint(data.data(), data.size(), out, consumed, 6), VarintStatus::Overflow);
}
