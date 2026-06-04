// Known-answer tests for the Noise crypto stack:
//   * RFC 7748 X25519 + SHA-256 primitive vectors.
//   * The official cacophony Noise_NNpsk0_25519_ChaChaPoly_SHA256 vector,
//     exercising the full handshake + transport (mixing order, PSK handling,
//     HKDF, nonce byte-order and AEAD all at once).

#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/crypto/noise_handshake.hpp>
#include <esphome/api/crypto/primitives.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

namespace noise = esphome::api::noise;
using esphome::api::ByteBuffer;
using esphome::api::ByteView;

namespace {

ByteBuffer unhex(const std::string& hex) {
    ByteBuffer out;
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
}

std::string tohex(const ByteView data) {
    static const auto* digits = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (std::size_t i = 0; i < data.size(); ++i) {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 0x0F]);
    }
    return out;
}

template <std::size_t N>
std::array<std::uint8_t, N> to_array(const ByteBuffer& b) {
    std::array<std::uint8_t, N> a{};
    for (std::size_t i = 0; i < N && i < b.size(); ++i) {
        a[i] = b[i];
    }
    return a;
}

ByteView view(const ByteBuffer& b) {
    return ByteView(b.data(), b.size());
}

}  // namespace

TEST(NoisePrimitives, Sha256KnownAnswer) {
    const ByteBuffer abc = {'a', 'b', 'c'};
    const noise::Hash h = noise::sha256(abc);
    EXPECT_EQ(tohex(ByteView(h.data(), h.size())),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(NoisePrimitives, X25519Rfc7748) {
    // RFC 7748 §6.1 Alice/Bob.
    const auto alice_priv =
        to_array<32>(unhex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a"));
    const auto bob_priv =
        to_array<32>(unhex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb"));

    const auto alice_pub = noise::x25519_base(alice_priv);
    EXPECT_EQ(tohex(view({alice_pub.begin(), alice_pub.end()})),
              "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a");

    const auto bob_pub = noise::x25519_base(bob_priv);
    const auto shared = noise::x25519(alice_priv, bob_pub);
    EXPECT_EQ(tohex(view({shared.begin(), shared.end()})),
              "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742");
}

// The full ESPHome handshake protocol against the official cacophony vector.
TEST(NoiseHandshake, CacophonyNNpsk0Vector) {
    const ByteBuffer prologue = unhex("4a6f686e2047616c74");
    const auto psk =
        to_array<32>(unhex("54686973206973206d7920417573747269616e20706572737065637469766521"));
    const auto init_eph =
        to_array<32>(unhex("893e28b9dc6ca8d611ab664754b8ceb7bac5117349a4439a6b0569da977c464a"));
    const auto resp_eph =
        to_array<32>(unhex("bbdb4cdbd309f1a1f2e1456967fe288cadd6f712d65dc7b7793d5e63da6b375b"));
    const std::string handshake_hash =
        "f4d03dc34495c95729ea6de9e1b59004b59733102488b3e24bc441e0be208eaf";

    noise::NoiseHandshake initiator;
    noise::NoiseHandshake responder;
    initiator.initialize(noise::NoiseHandshake::Role::Initiator, psk, view(prologue));
    responder.initialize(noise::NoiseHandshake::Role::Responder, psk, view(prologue));
    initiator.set_ephemeral_for_testing(init_eph);
    responder.set_ephemeral_for_testing(resp_eph);

    // Message 0: initiator -> responder.
    const ByteBuffer payload0 = unhex("4c756477696720766f6e204d69736573");
    const ByteBuffer msg0 = initiator.write_message(view(payload0));
    EXPECT_EQ(tohex(view(msg0)),
              "ca35def5ae56cec33dc2036731ab14896bc4c75dbb07a61f879f8e3afa4c7944"
              "79b962b8aff8485742ac32f905ba45369e2465fb59e138a93d67a0d1266b6a54");

    ByteBuffer got0;
    ASSERT_TRUE(responder.read_message(view(msg0), got0));
    EXPECT_EQ(tohex(view(got0)), tohex(view(payload0)));

    // Message 1: responder -> initiator (completes the handshake).
    const ByteBuffer payload1 = unhex("4d757272617920526f746862617264");
    const ByteBuffer msg1 = responder.write_message(view(payload1));
    EXPECT_EQ(tohex(view(msg1)),
              "95ebc60d2b1fa672c1f46a8aa265ef51bfe38e7ccb39ec5be34069f144808843"
              "d6062704d5a9c422a8e834423f8c1feada7e8d0d910a1a2cd030fb584221e3");

    ByteBuffer got1;
    ASSERT_TRUE(initiator.read_message(view(msg1), got1));
    EXPECT_EQ(tohex(view(got1)), tohex(view(payload1)));

    ASSERT_TRUE(initiator.is_complete());
    ASSERT_TRUE(responder.is_complete());
    EXPECT_EQ(tohex(view({initiator.handshake_hash().begin(), initiator.handshake_hash().end()})),
              handshake_hash);
    EXPECT_EQ(tohex(view({responder.handshake_hash().begin(), responder.handshake_hash().end()})),
              handshake_hash);

    // Transport phase: split and exchange. Cacophony alternates directions.
    noise::CipherState init_send;
    noise::CipherState init_recv;
    noise::CipherState resp_send;
    noise::CipherState resp_recv;
    initiator.split(init_send, init_recv);
    responder.split(resp_send, resp_recv);

    struct TransportMsg {
        bool from_initiator;
        const char* payload;
        const char* ciphertext;
    };
    constexpr std::array<TransportMsg, 4> msgs = {{
        {true, "462e20412e20486179656b", "e632c3763d7669067383433197a3baddf146e9e70ad4b4e9e59e0f"},
        {false, "4361726c204d656e676572", "64c6bee32ea91c8474bb4c21d7a700109ad45af77b29764ba5eb1e"},
        {true,
         "4a65616e2d426170746973746520536179",
         "e2fa0bed0603b62d3ccac2ecabbf3fe33f3e86514909b323361626266cb2471cc8"},
        {false,
         "457567656e2042f6686d20766f6e2042617765726b",
         "0c01dc9cec1fe4ddd692e8dd32188aa351088dc91183639a53b57aa4692b5ebdef8b8ca111"},
    }};

    for (const auto& tm : msgs) {
        const ByteBuffer payload = unhex(tm.payload);
        noise::CipherState& sender = tm.from_initiator ? init_send : resp_send;
        noise::CipherState& receiver = tm.from_initiator ? resp_recv : init_recv;

        const ByteBuffer ct = sender.encrypt_with_ad(ByteView{}, view(payload));
        EXPECT_EQ(tohex(view(ct)), tm.ciphertext);

        const ByteBuffer pt = receiver.decrypt_with_ad(ByteView{}, view(ct));
        EXPECT_EQ(tohex(view(pt)), tm.payload);
    }
}

// Sanity round-trip with random ephemerals (no fixed vector).
TEST(NoiseHandshake, RandomRoundTrip) {
    noise::SymmetricKey psk{};
    for (std::size_t i = 0; i < psk.size(); ++i) {
        psk[i] = static_cast<std::uint8_t>(i * 7 + 1);
    }
    const ByteBuffer prologue = {'N', 'o', 'i', 's', 'e', 'A', 'P', 'I', 'I', 'n', 'i', 't', 0, 0};

    noise::NoiseHandshake initiator;
    noise::NoiseHandshake responder;
    initiator.initialize(noise::NoiseHandshake::Role::Initiator, psk, view(prologue));
    responder.initialize(noise::NoiseHandshake::Role::Responder, psk, view(prologue));

    ByteBuffer p;
    const ByteBuffer m0 = initiator.write_message();
    ASSERT_TRUE(responder.read_message(view(m0), p));
    const ByteBuffer m1 = responder.write_message();
    ASSERT_TRUE(initiator.read_message(view(m1), p));
    ASSERT_TRUE(initiator.is_complete());

    noise::CipherState is;
    noise::CipherState ir;
    noise::CipherState rs;
    noise::CipherState rr;
    initiator.split(is, ir);
    responder.split(rs, rr);

    const ByteBuffer hello = {'h', 'e', 'l', 'l', 'o'};
    const ByteBuffer ct = is.encrypt_with_ad(ByteView{}, view(hello));
    const ByteBuffer pt = rr.decrypt_with_ad(ByteView{}, view(ct));
    EXPECT_EQ(pt, hello);
}
