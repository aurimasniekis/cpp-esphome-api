#pragma once

/// @file
/// @brief Bluetooth proxy subsystem: BLE scanning, connections, and GATT.

#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/subsystem.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace esphome::api {

/// Service or manufacturer data blob attached to an advertisement.
struct BleServiceData {
    std::string uuid;
    std::string data;
};

/// Manufacturer-specific data blob attached to an advertisement.
struct BleManufacturerData {
    std::string uuid;
    std::string data;
};

/// A parsed BLE advertisement (api: `BluetoothLEAdvertisementResponse`).
struct BleAdvertisement {
    std::uint64_t address = 0;
    std::string name;
    std::int32_t rssi = 0;
    std::uint32_t address_type = 0;
    std::vector<BleServiceData> service_data;
    std::vector<std::string> service_uuids;
    std::vector<BleManufacturerData> manufacturer_data;
};

/// A single raw, unparsed BLE advertisement (api: `BluetoothLERawAdvertisement`).
struct BleRawAdvertisement {
    std::uint64_t address = 0;
    std::int32_t rssi = 0;
    std::uint32_t address_type = 0;
    std::string data;
};

/// State of the device's BLE scanner (api: `BluetoothScannerStateResponse`).
struct BleScannerState {
    BluetoothScannerState state = BluetoothScannerState::Idle;
    BluetoothScannerMode mode = BluetoothScannerMode::Passive;
    BluetoothScannerMode configured_mode = BluetoothScannerMode::Passive;
};

/// Connection state update for a device (api: `BluetoothDeviceConnectionResponse`).
struct BleConnection {
    std::uint64_t address = 0;
    bool connected = false;
    std::uint32_t mtu = 0;
    std::int32_t error = 0;
};

/// Number of free connection slots (api: `BluetoothConnectionsFreeResponse`).
struct BleConnectionsFree {
    std::uint32_t free = 0;
    std::uint32_t limit = 0;
    std::vector<std::uint64_t> allocated;
};

/// Result of a GATT characteristic read (api: `BluetoothGATTReadResponse`).
struct BleGattRead {
    std::uint64_t address = 0;
    std::uint32_t handle = 0;
    std::string data;
};

/// Notification payload from a GATT characteristic
/// (api: `BluetoothGATTNotifyDataResponse`).
struct BleGattNotifyData {
    std::uint64_t address = 0;
    std::uint32_t handle = 0;
    std::string data;
};

/// One descriptor within a GATT characteristic.
struct BleGattDescriptor {
    std::vector<std::uint64_t> uuid;
    std::uint32_t handle = 0;
};

/// Typed view of the standard BLE GATT characteristic property bits.
struct BleGattCharProperties {
    bool broadcast = false;                    ///< 0x01
    bool read = false;                         ///< 0x02
    bool write_without_response = false;       ///< 0x04
    bool write = false;                        ///< 0x08
    bool notify = false;                       ///< 0x10
    bool indicate = false;                     ///< 0x20
    bool authenticated_signed_writes = false;  ///< 0x40
    bool extended_properties = false;          ///< 0x80

    [[nodiscard]] static BleGattCharProperties from_bits(const std::uint32_t b) {
        return {(b & 0x01) != 0,
                (b & 0x02) != 0,
                (b & 0x04) != 0,
                (b & 0x08) != 0,
                (b & 0x10) != 0,
                (b & 0x20) != 0,
                (b & 0x40) != 0,
                (b & 0x80) != 0};
    }
};

/// One characteristic within a GATT service.
struct BleGattCharacteristic {
    std::vector<std::uint64_t> uuid;
    std::uint32_t handle = 0;
    std::uint32_t properties = 0;  ///< Raw property bitmask (advanced).
    std::vector<BleGattDescriptor> descriptors;

    /// Typed view of `properties`.
    [[nodiscard]] BleGattCharProperties props() const {
        return BleGattCharProperties::from_bits(properties);
    }
};

/// One GATT service exposed by a device.
struct BleGattService {
    std::vector<std::uint64_t> uuid;
    std::uint32_t handle = 0;
    std::vector<BleGattCharacteristic> characteristics;
};

/// A batch of GATT services for a device
/// (api: `BluetoothGATTGetServicesResponse`).
struct BleGattServices {
    std::uint64_t address = 0;
    std::vector<BleGattService> services;
};

/// A GATT operation error (api: `BluetoothGATTErrorResponse`).
struct BleGattError {
    std::uint64_t address = 0;
    std::uint32_t handle = 0;
    std::int32_t error = 0;
};

/// Result of a one-shot GATT read: either `data` (ok) or `error` (!ok).
struct BleGattReadResult {
    std::uint64_t address = 0;
    std::uint32_t handle = 0;
    bool ok = false;
    std::string data;
    std::int32_t error = 0;
};

/// Result of a one-shot GATT write.
struct BleGattWriteResult {
    std::uint64_t address = 0;
    std::uint32_t handle = 0;
    bool ok = false;
    std::int32_t error = 0;
};

/// Result of a one-shot GATT service discovery (all batches collected).
struct BleGattServicesResult {
    std::uint64_t address = 0;
    bool ok = false;
    std::vector<BleGattService> services;
    std::int32_t error = 0;
};

/// Scans for, connects to, and talks GATT to BLE devices through the device's
/// Bluetooth proxy.
class BluetoothProxy : public Subsystem {
public:
    using AdvertisementHandler = std::function<void(const BleAdvertisement&)>;
    using RawAdvertisementHandler = std::function<void(const std::vector<BleRawAdvertisement>&)>;
    using ScannerStateHandler = std::function<void(const BleScannerState&)>;
    using ConnectionHandler = std::function<void(const BleConnection&)>;
    using ConnectionsFreeHandler = std::function<void(const BleConnectionsFree&)>;
    using GattReadHandler = std::function<void(const BleGattRead&)>;
    using GattNotifyDataHandler = std::function<void(const BleGattNotifyData&)>;
    using GattServicesHandler = std::function<void(const BleGattServices&)>;
    using GattServicesDoneHandler = std::function<void(std::uint64_t address)>;
    using GattErrorHandler = std::function<void(const BleGattError&)>;
    using ReadResultHandler = std::function<void(const BleGattReadResult&)>;
    using WriteResultHandler = std::function<void(const BleGattWriteResult&)>;
    using ServicesResultHandler = std::function<void(const BleGattServicesResult&)>;

    explicit BluetoothProxy(Client& client) : Subsystem(client) {}

    // --- Scanning ---------------------------------------------------------

    /// Subscribe to parsed BLE advertisements. `handler` receives each one.
    /// `flags` is passed through to the device (0 by default).
    void subscribe_advertisements(AdvertisementHandler handler, std::uint32_t flags = 0);

    /// Subscribe to raw (unparsed) BLE advertisement batches.
    void on_raw_advertisements(RawAdvertisementHandler handler);

    /// Stop receiving BLE advertisements.
    void unsubscribe_advertisements() const;

    /// Set the scanner mode (passive or active).
    void set_scanner_mode(BluetoothScannerMode mode) const;

    /// Register a callback for scanner state changes.
    void on_scanner_state(ScannerStateHandler handler);

    // --- Connections ------------------------------------------------------

    /// Ask the proxy to connect to `address`. `with_cache` selects the
    /// V3-with-cache vs V3-without-cache request type.
    void request_connection(std::uint64_t address, bool with_cache = true) const;

    /// Disconnect from `address`.
    void disconnect(std::uint64_t address) const;

    /// Subscribe to the number of free connection slots.
    void subscribe_connections_free() const;

    /// Register a callback for connection state changes.
    void on_connection(ConnectionHandler handler);

    /// Register a callback for free-connection-slot updates.
    void on_connections_free(ConnectionsFreeHandler handler);

    // --- GATT (one-shot, request/response) --------------------------------
    // These correlate the device's response to your call and deliver it (or a
    // GATT error) to `done`. Drive the event loop to receive it.

    /// Read a GATT characteristic value; `done` fires once with the result.
    void read(std::uint64_t address, std::uint32_t handle, ReadResultHandler done);

    /// Write a GATT characteristic value; `done` fires once with the result.
    void write(std::uint64_t address,
               std::uint32_t handle,
               const std::string& data,
               bool response,
               WriteResultHandler done);

    /// Discover the full GATT service table; `done` fires once when complete.
    void get_services(std::uint64_t address, ServicesResultHandler done);

    // --- GATT (raw / advanced, fire-and-forget) ---------------------------
    // Send the request and observe responses via the on_* streaming callbacks.

    /// Read a GATT characteristic value (response via on_read / on_error).
    void read(std::uint64_t address, std::uint32_t handle) const;

    /// Write a GATT characteristic value. `response` requests a write response.
    void write(std::uint64_t address,
               std::uint32_t handle,
               const std::string& data,
               bool response = true) const;

    /// Enable or disable notifications for a GATT characteristic.
    void notify(std::uint64_t address, std::uint32_t handle, bool enable) const;

    /// Request the GATT service table (responses via on_services / on_services_done).
    void get_services(std::uint64_t address) const;

    /// Register a callback for characteristic read results.
    void on_read(GattReadHandler handler);

    /// Register a callback for characteristic notification payloads.
    void on_notify_data(GattNotifyDataHandler handler);

    /// Register a callback for batches of GATT services.
    void on_services(GattServicesHandler handler);

    /// Register a callback fired when the GATT service table is fully sent.
    void on_services_done(GattServicesDoneHandler handler);

    /// Register a callback for GATT operation errors.
    void on_error(GattErrorHandler handler);

private:
    /// Register the shared GATT response dispatchers once (lazily). They feed
    /// both the persistent on_* handlers and the one-shot pending tables.
    void ensure_gatt_handlers();
    static std::uint64_t gatt_key(const std::uint64_t address, const std::uint32_t handle) {
        return (address << 16) | (handle & 0xFFFFU);
    }

    AdvertisementHandler advertisement_handler_;
    RawAdvertisementHandler raw_advertisement_handler_;
    ScannerStateHandler scanner_state_handler_;
    ConnectionHandler connection_handler_;
    ConnectionsFreeHandler connections_free_handler_;
    GattReadHandler read_handler_;
    GattNotifyDataHandler notify_data_handler_;
    GattServicesHandler services_handler_;
    GattServicesDoneHandler services_done_handler_;
    GattErrorHandler error_handler_;

    bool gatt_registered_ = false;
    std::unordered_map<std::uint64_t, std::vector<ReadResultHandler>> pending_reads_;
    std::unordered_map<std::uint64_t, std::vector<WriteResultHandler>> pending_writes_;
    struct ServicesAccumulator {
        std::vector<BleGattService> services;
        std::vector<ServicesResultHandler> handlers;
    };
    std::unordered_map<std::uint64_t, ServicesAccumulator> pending_services_;
};

}  // namespace esphome::api
