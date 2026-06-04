// Integration tests for subsystems over the real stack + loopback server.

#include <esphome/api/exception.hpp>
#include <esphome/api/subsystems/bluetooth_proxy.hpp>
#include <esphome/api/subsystems/log_stream.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>
#include <esphome/api/sync_client.hpp>

#include <gtest/gtest.h>

#include "support/loopback_server.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using esphome::api::BleAdvertisement;
using esphome::api::BluetoothProxy;
using esphome::api::ClientOptions;
using esphome::api::LogEntry;
using esphome::api::LogLevel;
using esphome::api::LogStream;
using esphome::api::SyncClient;
using esphome::api::testing::LoopbackServer;

namespace {
ClientOptions local_options(const std::uint16_t port) {
    ClientOptions opt;
    opt.host = "127.0.0.1";
    opt.port = port;
    opt.connection.connect_timeout = std::chrono::seconds(5);
    return opt;
}
}  // namespace

TEST(Subsystems, LogStreamReceivesEntries) {
    LoopbackServer server;
    server.start();

    SyncClient client(local_options(server.port()));
    client.connect();

    std::vector<LogEntry> entries;
    LogStream logs(client.async());
    logs.subscribe(LogLevel::VeryVerbose, [&](const LogEntry& e) { entries.push_back(e); });

    try {
        client.pump_until([&] { return !entries.empty(); }, std::chrono::seconds(3));
    } catch (...) {}

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries[0].message, "hello from loopback");
    EXPECT_EQ(entries[0].level, LogLevel::Info);
    client.disconnect();
}

TEST(Subsystems, BluetoothProxyReceivesAdvertisement) {
    LoopbackServer server;
    server.start();

    SyncClient client(local_options(server.port()));
    client.connect();

    std::vector<BleAdvertisement> ads;
    BluetoothProxy ble(client.async());
    ble.subscribe_advertisements([&](const BleAdvertisement& a) { ads.push_back(a); });

    try {
        client.pump_until([&] { return !ads.empty(); }, std::chrono::seconds(3));
    } catch (...) {}

    ASSERT_FALSE(ads.empty());
    EXPECT_EQ(ads[0].address, 0x112233445566ULL);
    EXPECT_EQ(ads[0].name, "ble-device");
    EXPECT_EQ(ads[0].rssi, -42);
    client.disconnect();
}

TEST(Subsystems, SerialProxyModemPinsOneShot) {
    LoopbackServer server;
    server.start();

    SyncClient client(local_options(server.port()));
    client.connect();

    esphome::api::SerialProxyModemPins got;
    bool fired = false;
    // Instance-bound handle: no instance argument on the call.
    auto port = client.serial_proxy(3);
    port.get_modem_pins([&](const esphome::api::SerialProxyModemPins& pins) {
        got = pins;
        fired = true;
    });

    try {
        client.pump_until([&] { return fired; }, std::chrono::seconds(2));
    } catch (...) {}

    ASSERT_TRUE(fired);
    EXPECT_EQ(got.instance, 3U);
    EXPECT_TRUE(got.lines.rts);  // typed flags instead of a raw uint32
    EXPECT_TRUE(got.lines.dtr);

    // Round-trip the typed flags through the bitmask.
    esphome::api::SerialProxyLineStates ls{true, false};
    EXPECT_EQ(ls.to_bits(), esphome::api::SerialProxyLineStates::rts_bit);
    EXPECT_TRUE(esphome::api::SerialProxyLineStates::from_bits(0x3).dtr);

    client.disconnect();
}

TEST(Subsystems, SerialProxyByName) {
    LoopbackServer server;
    server.start();

    SyncClient client(local_options(server.port()));
    client.connect();

    // Resolves the name via device info (fetched lazily) to instance index.
    auto port = client.serial_proxy("rs485-bus");
    EXPECT_EQ(port.index(), 1U);
    EXPECT_EQ(client.serial_proxy("console").index(), 0U);
    EXPECT_THROW((void)client.serial_proxy("nope"), esphome::api::ApiError);

    client.disconnect();
}

TEST(Subsystems, BluetoothGattReadOneShot) {
    LoopbackServer server;
    server.start();

    SyncClient client(local_options(server.port()));
    client.connect();

    esphome::api::BleGattReadResult got;
    bool fired = false;
    client.bluetooth().read(0xABCDEF, 0x2A, [&](const esphome::api::BleGattReadResult& r) {
        got = r;
        fired = true;
    });

    try {
        client.pump_until([&] { return fired; }, std::chrono::seconds(2));
    } catch (...) {}

    ASSERT_TRUE(fired);
    EXPECT_TRUE(got.ok);
    EXPECT_EQ(got.address, 0xABCDEFULL);
    EXPECT_EQ(got.handle, 0x2AU);
    EXPECT_EQ(got.data, "gatt-value");

    // Typed characteristic properties.
    esphome::api::BleGattCharacteristic chr;
    chr.properties = 0x12;  // read | notify
    EXPECT_TRUE(chr.props().read);
    EXPECT_TRUE(chr.props().notify);
    EXPECT_FALSE(chr.props().write);

    client.disconnect();
}
