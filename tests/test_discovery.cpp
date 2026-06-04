// Unit test for the mDNS response parser (no sockets).

#include <esphome/api/discovery.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using esphome::api::ByteBuffer;
using esphome::api::DiscoveredDevice;
using esphome::api::parse_mdns_packets;

namespace {

void u16(ByteBuffer& b, const std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>(v >> 8));
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
}
void u32(ByteBuffer& b, const std::uint32_t v) {
    b.push_back(static_cast<std::uint8_t>(v >> 24));
    b.push_back(static_cast<std::uint8_t>(v >> 16));
    b.push_back(static_cast<std::uint8_t>(v >> 8));
    b.push_back(static_cast<std::uint8_t>(v));
}
void enc_name(ByteBuffer& b, const std::string& dotted) {
    std::size_t i = 0;
    while (i < dotted.size()) {
        std::size_t j = dotted.find('.', i);
        if (j == std::string::npos) {
            j = dotted.size();
        }
        b.push_back(static_cast<std::uint8_t>(j - i));
        b.insert(
            b.end(), dotted.begin() + static_cast<long>(i), dotted.begin() + static_cast<long>(j));
        i = j + 1;
    }
    b.push_back(0);
}
void txt_entry(ByteBuffer& rd, const std::string& kv) {
    rd.push_back(static_cast<std::uint8_t>(kv.size()));
    rd.insert(rd.end(), kv.begin(), kv.end());
}

// Build one mDNS response packet describing a device, using a compression
// pointer for the A record's name to exercise that code path.
ByteBuffer build_packet() {
    const std::string instance = "living-room._esphomelib._tcp.local";
    const std::string host = "living-room.local";

    ByteBuffer p;
    u16(p, 0);       // id
    u16(p, 0x8400);  // flags: response + authoritative
    u16(p, 0);       // qd
    u16(p, 4);       // an
    u16(p, 0);       // ns
    u16(p, 0);       // ar

    // PTR _esphomelib._tcp.local -> instance
    enc_name(p, "_esphomelib._tcp.local");
    u16(p, 12);  // PTR
    u16(p, 1);   // IN
    u32(p, 120);
    ByteBuffer ptr_rd;
    enc_name(ptr_rd, instance);
    u16(p, static_cast<std::uint16_t>(ptr_rd.size()));
    p.insert(p.end(), ptr_rd.begin(), ptr_rd.end());

    // SRV instance -> host:6053 (records the host name offset for compression)
    enc_name(p, instance);
    u16(p, 33);  // SRV
    u16(p, 1);
    u32(p, 120);
    ByteBuffer srv_rd;
    u16(srv_rd, 0);     // priority
    u16(srv_rd, 0);     // weight
    u16(srv_rd, 6053);  // port
    const std::size_t host_offset =
        p.size() + 2 + srv_rd.size();  // where the SRV target name starts
    enc_name(srv_rd, host);
    u16(p, static_cast<std::uint16_t>(srv_rd.size()));
    p.insert(p.end(), srv_rd.begin(), srv_rd.end());

    // TXT instance -> key/value records
    enc_name(p, instance);
    u16(p, 16);  // TXT
    u16(p, 1);
    u32(p, 120);
    ByteBuffer txt_rd;
    txt_entry(txt_rd, "version=2024.12.0");
    txt_entry(txt_rd, "mac=aabbccddeeff");
    txt_entry(txt_rd, "friendly_name=Living Room");
    txt_entry(txt_rd, "platform=ESP32");
    txt_entry(txt_rd, "api_encryption=Noise_NNpsk0_25519_ChaChaPoly_SHA256");
    u16(p, static_cast<std::uint16_t>(txt_rd.size()));
    p.insert(p.end(), txt_rd.begin(), txt_rd.end());

    // A record whose NAME is a compression pointer to the host name above.
    p.push_back(static_cast<std::uint8_t>(0xC0 | (host_offset >> 8)));
    p.push_back(static_cast<std::uint8_t>(host_offset & 0xFF));
    u16(p, 1);  // A
    u16(p, 1);
    u32(p, 120);
    u16(p, 4);  // rdlength
    p.push_back(192);
    p.push_back(168);
    p.push_back(0);
    p.push_back(225);

    return p;
}

}  // namespace

TEST(Discovery, ParsesDeviceFromPacket) {
    const std::vector<ByteBuffer> packets = {build_packet()};
    const auto devices = parse_mdns_packets(packets);

    ASSERT_EQ(devices.size(), 1U);
    const DiscoveredDevice& d = devices[0];
    EXPECT_EQ(d.name, "living-room");
    EXPECT_EQ(d.hostname, "living-room.local");
    EXPECT_EQ(d.address, "192.168.0.225");  // resolved via the compressed A record
    EXPECT_EQ(d.port, 6053);
    EXPECT_EQ(d.mac, "aabbccddeeff");
    EXPECT_EQ(d.version, "2024.12.0");
    EXPECT_EQ(d.friendly_name, "Living Room");
    EXPECT_EQ(d.platform, "ESP32");
    EXPECT_TRUE(d.requires_encryption);
    EXPECT_TRUE(d.supports_encryption);
    EXPECT_EQ(d.connect_host(), "192.168.0.225");
    EXPECT_EQ(d.properties.at("api_encryption"), "Noise_NNpsk0_25519_ChaChaPoly_SHA256");
}

TEST(Discovery, IgnoresUnrelatedAndEmpty) {
    EXPECT_TRUE(parse_mdns_packets({}).empty());
    ByteBuffer junk = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // header only, no records
    EXPECT_TRUE(parse_mdns_packets({junk}).empty());
}
