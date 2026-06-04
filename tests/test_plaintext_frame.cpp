#include <esphome/api/exception.hpp>
#include <esphome/api/frame/plaintext_frame_helper.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using esphome::api::ByteBuffer;
using esphome::api::ByteView;
using esphome::api::EncryptionMismatchError;
using esphome::api::FrameStatus;
using esphome::api::PlaintextFrameHelper;
using esphome::api::ProtocolError;

namespace {

ByteBuffer make_payload(const std::size_t n, const std::uint8_t seed = 0) {
    ByteBuffer b;
    b.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        b.push_back(static_cast<std::uint8_t>(i + seed));
    }
    return b;
}

struct Decoded {
    std::uint32_t type;
    ByteBuffer payload;
};

}  // namespace

TEST(PlaintextFrame, EncodeShape) {
    PlaintextFrameHelper fh;
    ByteBuffer out;
    const ByteBuffer payload = {0xDE, 0xAD};
    fh.encode(42, payload, out);
    // [0x00][len=2][type=42][payload...]
    ASSERT_EQ(out.size(), 1U + 1U + 1U + 2U);
    EXPECT_EQ(out[0], 0x00U);
    EXPECT_EQ(out[1], 0x02U);
    EXPECT_EQ(out[2], 42U);
    EXPECT_EQ(out[3], 0xDEU);
    EXPECT_EQ(out[4], 0xADU);
}

TEST(PlaintextFrame, EncodeDecodeRoundTrip) {
    PlaintextFrameHelper enc;
    PlaintextFrameHelper dec;
    ByteBuffer wire;
    const ByteBuffer payload = make_payload(100, 7);
    enc.encode(123, payload, wire);

    dec.feed(wire);
    std::uint32_t type = 0;
    ByteView view;
    ASSERT_EQ(dec.next(type, view), FrameStatus::Ok);
    EXPECT_EQ(type, 123U);
    ASSERT_EQ(view.size(), payload.size());
    EXPECT_EQ(ByteBuffer(view.begin(), view.end()), payload);
    // No second frame.
    EXPECT_EQ(dec.next(type, view), FrameStatus::NeedMore);
}

TEST(PlaintextFrame, ByteByBytePartialReads) {
    PlaintextFrameHelper enc;
    ByteBuffer wire;
    const ByteBuffer payload = make_payload(300);  // length needs a 2-byte varint
    enc.encode(7, payload, wire);

    PlaintextFrameHelper dec;
    std::uint32_t type = 0;
    ByteView view;
    // Feed all but the last byte one at a time: always NeedMore.
    for (std::size_t i = 0; i + 1 < wire.size(); ++i) {
        dec.feed(ByteView(&wire[i], 1));
        EXPECT_EQ(dec.next(type, view), FrameStatus::NeedMore) << "at byte " << i;
    }
    // Final byte completes the frame.
    dec.feed(ByteView(&wire[wire.size() - 1], 1));
    ASSERT_EQ(dec.next(type, view), FrameStatus::Ok);
    EXPECT_EQ(type, 7U);
    EXPECT_EQ(ByteBuffer(view.begin(), view.end()), payload);
}

TEST(PlaintextFrame, MultipleFramesInOneFeed) {
    PlaintextFrameHelper enc;
    ByteBuffer wire;
    enc.encode(1, make_payload(10, 1), wire);
    enc.encode(2, make_payload(20, 2), wire);
    enc.encode(3, make_payload(0), wire);  // empty payload

    PlaintextFrameHelper dec;
    dec.feed(wire);

    std::vector<Decoded> frames;
    std::uint32_t type = 0;
    ByteView view;
    while (dec.next(type, view) == FrameStatus::Ok) {
        frames.push_back({type, ByteBuffer(view.begin(), view.end())});
    }
    ASSERT_EQ(frames.size(), 3U);
    EXPECT_EQ(frames[0].type, 1U);
    EXPECT_EQ(frames[0].payload, make_payload(10, 1));
    EXPECT_EQ(frames[1].type, 2U);
    EXPECT_EQ(frames[1].payload, make_payload(20, 2));
    EXPECT_EQ(frames[2].type, 3U);
    EXPECT_TRUE(frames[2].payload.empty());
}

TEST(PlaintextFrame, SplitAcrossTwoChunks) {
    PlaintextFrameHelper enc;
    ByteBuffer wire;
    enc.encode(9, make_payload(50, 3), wire);
    enc.encode(10, make_payload(50, 4), wire);

    PlaintextFrameHelper dec;
    std::uint32_t type = 0;
    ByteView view;

    // Feed a chunk that contains the first frame plus part of the second.
    const std::size_t split = wire.size() - 10;
    dec.feed(ByteView(wire.data(), split));
    ASSERT_EQ(dec.next(type, view), FrameStatus::Ok);
    EXPECT_EQ(type, 9U);
    EXPECT_EQ(dec.next(type, view), FrameStatus::NeedMore);

    dec.feed(ByteView(wire.data() + split, wire.size() - split));
    ASSERT_EQ(dec.next(type, view), FrameStatus::Ok);
    EXPECT_EQ(type, 10U);
}

TEST(PlaintextFrame, NoiseIndicatorReportsEncryptionRequired) {
    PlaintextFrameHelper dec;
    ByteBuffer bad = {0x01, 0x00, 0x00};  // Noise indicator on a plaintext helper
    dec.feed(bad);
    std::uint32_t type = 0;
    ByteView view;
    // A device that requires Noise is a specific, actionable case — not a
    // generic framing error — so a dedicated exception carries the hint.
    EXPECT_THROW(dec.next(type, view), EncryptionMismatchError);
    try {
        dec.next(type, view);
        FAIL() << "expected EncryptionMismatchError";
    } catch (const EncryptionMismatchError& e) {
        EXPECT_NE(std::string(e.what()).find("--key"), std::string::npos);
    }
}

TEST(PlaintextFrame, OtherBadIndicatorThrowsProtocolError) {
    PlaintextFrameHelper dec;
    ByteBuffer bad = {0x42, 0x00, 0x00};  // neither plaintext (0x00) nor Noise (0x01)
    dec.feed(bad);
    std::uint32_t type = 0;
    ByteView view;
    EXPECT_THROW(dec.next(type, view), ProtocolError);
}

TEST(PlaintextFrame, OversizedLengthVarintThrows) {
    PlaintextFrameHelper dec;
    // 0x00 indicator then a 5-byte length varint (exceeds the 4-byte budget).
    ByteBuffer bad = {0x00, 0x80, 0x80, 0x80, 0x80, 0x01};
    dec.feed(bad);
    std::uint32_t type = 0;
    ByteView view;
    EXPECT_THROW(dec.next(type, view), ProtocolError);
}
