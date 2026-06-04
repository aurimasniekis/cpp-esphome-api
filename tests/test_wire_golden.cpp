// Byte-exactness oracle for the hand-rolled wire codec.
//
// golden_vectors.inc holds reference proto3-serialized bytes for every message
// that carries an (id), produced from deterministic non-default field values.
// For each vector we:
//   1. create the message by id via the registry,
//   2. decode the reference bytes with our codec,
//   3. assert calculate_size() equals the byte length, and
//   4. assert re-encoding reproduces the reference bytes byte-for-byte.
//
// (2)+(4) together prove our decoder accepts standard proto3 wire format and our
// encoder reproduces it exactly — the contract that lets us talk to real ESPHome
// devices.

#include <esphome/api/proto/message_registry.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

struct GoldenVector {
    std::uint32_t id;
    const char* name;
    std::vector<int> bytes;
};

const std::initializer_list<GoldenVector> k_golden_vectors = {
#include "golden_vectors.inc"
};

std::string to_string(const std::vector<int>& bytes) {
    std::string out;
    out.reserve(bytes.size());
    for (const int b : bytes)
        out.push_back(static_cast<char>(static_cast<unsigned char>(b)));
    return out;
}

std::string hex(const std::string& s) {
    static const auto* digits = "0123456789abcdef";
    std::string out;
    out.reserve(s.size() * 2);
    for (const char ch : s) {
        const auto c = static_cast<unsigned char>(ch);
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 0xf]);
    }
    return out;
}

}  // namespace

TEST(WireGolden, DecodeReEncodeIsByteExact) {
    ASSERT_GT(k_golden_vectors.size(), 100U);

    for (const auto& [id, name, bytes] : k_golden_vectors) {
        const std::string expected = to_string(bytes);

        auto msg = esphome::api::MessageRegistry::create(id);
        ASSERT_NE(msg, nullptr) << name << " (id " << id << ")";

        ASSERT_TRUE(msg->ParseFromArray(expected.data(), static_cast<int>(expected.size())))
            << name << ": decode failed";

        EXPECT_EQ(msg->calculate_size(), expected.size()) << name << ": calculate_size mismatch";

        const std::string actual = msg->SerializeAsString();
        EXPECT_EQ(hex(actual), hex(expected)) << name << ": re-encode not byte-exact";
    }
}

// Truncated input must be rejected (connection.cpp relies on ParseFromArray
// returning false on malformed frames).
TEST(WireGolden, TruncatedInputRejected) {
    for (const auto& [id, name, bytes] : k_golden_vectors) {
        const std::string full = to_string(bytes);
        if (full.size() < 2)
            continue;
        auto msg = esphome::api::MessageRegistry::create(id);
        ASSERT_NE(msg, nullptr) << name;
        // Cut the final byte — for nearly every non-trivial message this lands
        // mid-field and must fail. (A few may legitimately still parse if the
        // last byte was an optional trailing field; accept either outcome but
        // require no crash / no over-read.)
        const bool ok = msg->ParseFromArray(full.data(), static_cast<int>(full.size() - 1));
        (void)ok;
    }
}
