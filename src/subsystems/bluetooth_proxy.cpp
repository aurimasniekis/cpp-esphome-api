#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/bluetooth_proxy.hpp>

#include "api.pb.h"

#include <utility>

namespace esphome::api {

namespace {

BleServiceData to_service_data(const proto::BluetoothServiceData& in) {
    BleServiceData out;
    out.uuid = in.uuid();
    out.data = in.data();
    return out;
}

BleGattService to_gatt_service(const proto::BluetoothGATTService& in) {
    BleGattService service;
    service.handle = in.handle();
    for (int i = 0; i < in.uuid_size(); ++i) {
        service.uuid.push_back(in.uuid(i));
    }
    for (int c = 0; c < in.characteristics_size(); ++c) {
        const auto& pc = in.characteristics(c);
        BleGattCharacteristic chr;
        chr.handle = pc.handle();
        chr.properties = pc.properties();
        for (int i = 0; i < pc.uuid_size(); ++i) {
            chr.uuid.push_back(pc.uuid(i));
        }
        for (int d = 0; d < pc.descriptors_size(); ++d) {
            const auto& pd = pc.descriptors(d);
            BleGattDescriptor desc;
            desc.handle = pd.handle();
            for (int i = 0; i < pd.uuid_size(); ++i) {
                desc.uuid.push_back(pd.uuid(i));
            }
            chr.descriptors.push_back(std::move(desc));
        }
        service.characteristics.push_back(std::move(chr));
    }
    return service;
}

}  // namespace

// --- Scanning -------------------------------------------------------------

void BluetoothProxy::subscribe_advertisements(AdvertisementHandler handler,
                                              const std::uint32_t flags) {
    advertisement_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::BluetoothLEAdvertisementResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::BluetoothLEAdvertisementResponse&>(msg);
        if (!advertisement_handler_) {
            return;
        }
        BleAdvertisement adv;
        adv.address = resp.address();
        adv.name = resp.name();
        adv.rssi = resp.rssi();
        adv.address_type = resp.address_type();
        for (int i = 0; i < resp.service_uuids_size(); ++i) {
            adv.service_uuids.push_back(resp.service_uuids(i));
        }
        for (int i = 0; i < resp.service_data_size(); ++i) {
            adv.service_data.push_back(to_service_data(resp.service_data(i)));
        }
        for (int i = 0; i < resp.manufacturer_data_size(); ++i) {
            const auto& md = resp.manufacturer_data(i);
            adv.manufacturer_data.push_back(BleManufacturerData{md.uuid(), md.data()});
        }
        advertisement_handler_(adv);
    });

    proto::SubscribeBluetoothLEAdvertisementsRequest req;
    req.set_flags(flags);
    client_.send(req);
}

void BluetoothProxy::on_raw_advertisements(RawAdvertisementHandler handler) {
    raw_advertisement_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::BluetoothLERawAdvertisementsResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::BluetoothLERawAdvertisementsResponse&>(msg);
        if (!raw_advertisement_handler_) {
            return;
        }
        std::vector<BleRawAdvertisement> out;
        out.reserve(static_cast<std::size_t>(resp.advertisements_size()));
        for (int i = 0; i < resp.advertisements_size(); ++i) {
            const auto& a = resp.advertisements(i);
            BleRawAdvertisement raw;
            raw.address = a.address();
            raw.rssi = a.rssi();
            raw.address_type = a.address_type();
            raw.data = a.data();
            out.push_back(std::move(raw));
        }
        raw_advertisement_handler_(out);
    });
}

void BluetoothProxy::unsubscribe_advertisements() const {
    const proto::UnsubscribeBluetoothLEAdvertisementsRequest req;
    client_.send(req);
}

void BluetoothProxy::set_scanner_mode(BluetoothScannerMode mode) const {
    proto::BluetoothScannerSetModeRequest req;
    req.set_mode(static_cast<proto::BluetoothScannerMode>(mode));
    client_.send(req);
}

void BluetoothProxy::on_scanner_state(ScannerStateHandler handler) {
    scanner_state_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::BluetoothScannerStateResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::BluetoothScannerStateResponse&>(msg);
        if (scanner_state_handler_) {
            BleScannerState state;
            state.state = static_cast<BluetoothScannerState>(resp.state());
            state.mode = static_cast<BluetoothScannerMode>(resp.mode());
            state.configured_mode = static_cast<BluetoothScannerMode>(resp.configured_mode());
            scanner_state_handler_(state);
        }
    });
}

// --- Connections ----------------------------------------------------------

void BluetoothProxy::request_connection(const std::uint64_t address, const bool with_cache) const {
    proto::BluetoothDeviceRequest req;
    req.set_address(address);
    req.set_request_type(with_cache
                             ? proto::BLUETOOTH_DEVICE_REQUEST_TYPE_CONNECT_V3_WITH_CACHE
                             : proto::BLUETOOTH_DEVICE_REQUEST_TYPE_CONNECT_V3_WITHOUT_CACHE);
    client_.send(req);
}

void BluetoothProxy::disconnect(const std::uint64_t address) const {
    proto::BluetoothDeviceRequest req;
    req.set_address(address);
    req.set_request_type(proto::BLUETOOTH_DEVICE_REQUEST_TYPE_DISCONNECT);
    client_.send(req);
}

void BluetoothProxy::subscribe_connections_free() const {
    const proto::SubscribeBluetoothConnectionsFreeRequest req;
    client_.send(req);
}

void BluetoothProxy::on_connection(ConnectionHandler handler) {
    connection_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::BluetoothDeviceConnectionResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::BluetoothDeviceConnectionResponse&>(msg);
        if (connection_handler_) {
            BleConnection conn;
            conn.address = resp.address();
            conn.connected = resp.connected();
            conn.mtu = resp.mtu();
            conn.error = resp.error();
            connection_handler_(conn);
        }
    });
}

void BluetoothProxy::on_connections_free(ConnectionsFreeHandler handler) {
    connections_free_handler_ = std::move(handler);

    constexpr auto id = static_cast<std::uint32_t>(MessageId::BluetoothConnectionsFreeResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::BluetoothConnectionsFreeResponse&>(msg);
        if (connections_free_handler_) {
            BleConnectionsFree free_slots;
            free_slots.free = resp.free();
            free_slots.limit = resp.limit();
            for (int i = 0; i < resp.allocated_size(); ++i) {
                free_slots.allocated.push_back(resp.allocated(i));
            }
            connections_free_handler_(free_slots);
        }
    });
}

// --- GATT -----------------------------------------------------------------

void BluetoothProxy::ensure_gatt_handlers() {
    if (gatt_registered_) {
        return;
    }
    gatt_registered_ = true;

    client_.on(static_cast<std::uint32_t>(MessageId::BluetoothGATTReadResponse),
               [this](const ProtoMessage& msg) {
                   const auto& resp = static_cast<const proto::BluetoothGATTReadResponse&>(msg);
                   if (read_handler_) {
                       read_handler_(BleGattRead{resp.address(), resp.handle(), resp.data()});
                   }
                   if (const auto it = pending_reads_.find(gatt_key(resp.address(), resp.handle()));
                       it != pending_reads_.end()) {
                       const std::vector<ReadResultHandler> once = std::move(it->second);
                       pending_reads_.erase(it);
                       const BleGattReadResult r{
                           resp.address(), resp.handle(), true, resp.data(), 0};
                       for (const auto& cb : once) {
                           cb(r);
                       }
                   }
               });

    client_.on(static_cast<std::uint32_t>(MessageId::BluetoothGATTWriteResponse),
               [this](const ProtoMessage& msg) {
                   const auto& resp = static_cast<const proto::BluetoothGATTWriteResponse&>(msg);
                   if (const auto it =
                           pending_writes_.find(gatt_key(resp.address(), resp.handle()));
                       it != pending_writes_.end()) {
                       const std::vector<WriteResultHandler> once = std::move(it->second);
                       pending_writes_.erase(it);
                       const BleGattWriteResult r{resp.address(), resp.handle(), true, 0};
                       for (const auto& cb : once) {
                           cb(r);
                       }
                   }
               });

    client_.on(
        static_cast<std::uint32_t>(MessageId::BluetoothGATTNotifyDataResponse),
        [this](const ProtoMessage& msg) {
            const auto& resp = static_cast<const proto::BluetoothGATTNotifyDataResponse&>(msg);
            if (notify_data_handler_) {
                notify_data_handler_(BleGattNotifyData{resp.address(), resp.handle(), resp.data()});
            }
        });

    client_.on(static_cast<std::uint32_t>(MessageId::BluetoothGATTGetServicesResponse),
               [this](const ProtoMessage& msg) {
                   const auto& resp =
                       static_cast<const proto::BluetoothGATTGetServicesResponse&>(msg);
                   BleGattServices services;
                   services.address = resp.address();
                   for (int i = 0; i < resp.services_size(); ++i) {
                       services.services.push_back(to_gatt_service(resp.services(i)));
                   }
                   if (services_handler_) {
                       services_handler_(services);
                   }
                   auto& [acc_services, acc_handlers] = pending_services_[resp.address()];
                   for (auto& s : services.services) {
                       acc_services.push_back(std::move(s));
                   }
               });

    client_.on(
        static_cast<std::uint32_t>(MessageId::BluetoothGATTGetServicesDoneResponse),
        [this](const ProtoMessage& msg) {
            const auto& resp = static_cast<const proto::BluetoothGATTGetServicesDoneResponse&>(msg);
            if (services_done_handler_) {
                services_done_handler_(resp.address());
            }
            if (const auto it = pending_services_.find(resp.address());
                it != pending_services_.end()) {
                auto [acc_services, acc_handlers] = std::move(it->second);
                pending_services_.erase(it);
                const BleGattServicesResult r{resp.address(), true, std::move(acc_services), 0};
                for (auto& cb : acc_handlers) {
                    cb(r);
                }
            }
        });

    client_.on(
        static_cast<std::uint32_t>(MessageId::BluetoothGATTErrorResponse),
        [this](const ProtoMessage& msg) {
            const auto& resp = static_cast<const proto::BluetoothGATTErrorResponse&>(msg);
            if (error_handler_) {
                error_handler_(BleGattError{resp.address(), resp.handle(), resp.error()});
            }
            const std::uint64_t key = gatt_key(resp.address(), resp.handle());
            if (const auto it = pending_reads_.find(key); it != pending_reads_.end()) {
                const std::vector<ReadResultHandler> once = std::move(it->second);
                pending_reads_.erase(it);
                const BleGattReadResult r{resp.address(), resp.handle(), false, {}, resp.error()};
                for (const auto& cb : once) {
                    cb(r);
                }
            }
            if (const auto it = pending_writes_.find(key); it != pending_writes_.end()) {
                const std::vector<WriteResultHandler> once = std::move(it->second);
                pending_writes_.erase(it);
                const BleGattWriteResult r{resp.address(), resp.handle(), false, resp.error()};
                for (const auto& cb : once) {
                    cb(r);
                }
            }
            if (const auto it = pending_services_.find(resp.address());
                it != pending_services_.end()) {
                const auto [acc_services, acc_handlers] = std::move(it->second);
                pending_services_.erase(it);
                const BleGattServicesResult r{resp.address(), false, {}, resp.error()};
                for (const auto& cb : acc_handlers) {
                    cb(r);
                }
            }
        });
}

void BluetoothProxy::read(const std::uint64_t address,
                          const std::uint32_t handle,
                          ReadResultHandler done) {
    ensure_gatt_handlers();
    if (done) {
        pending_reads_[gatt_key(address, handle)].push_back(std::move(done));
    }
    read(address, handle);
}

void BluetoothProxy::write(const std::uint64_t address,
                           const std::uint32_t handle,
                           const std::string& data,
                           const bool response,
                           WriteResultHandler done) {
    ensure_gatt_handlers();
    if (done) {
        pending_writes_[gatt_key(address, handle)].push_back(std::move(done));
    }
    write(address, handle, data, response);
}

void BluetoothProxy::get_services(const std::uint64_t address, ServicesResultHandler done) {
    ensure_gatt_handlers();
    if (done) {
        pending_services_[address].handlers.push_back(std::move(done));
    }
    get_services(address);
}

void BluetoothProxy::read(const std::uint64_t address, const std::uint32_t handle) const {
    proto::BluetoothGATTReadRequest req;
    req.set_address(address);
    req.set_handle(handle);
    client_.send(req);
}

void BluetoothProxy::write(const std::uint64_t address,
                           const std::uint32_t handle,
                           const std::string& data,
                           const bool response) const {
    proto::BluetoothGATTWriteRequest req;
    req.set_address(address);
    req.set_handle(handle);
    req.set_response(response);
    req.set_data(data);
    client_.send(req);
}

void BluetoothProxy::notify(const std::uint64_t address,
                            const std::uint32_t handle,
                            const bool enable) const {
    proto::BluetoothGATTNotifyRequest req;
    req.set_address(address);
    req.set_handle(handle);
    req.set_enable(enable);
    client_.send(req);
}

void BluetoothProxy::get_services(const std::uint64_t address) const {
    proto::BluetoothGATTGetServicesRequest req;
    req.set_address(address);
    client_.send(req);
}

void BluetoothProxy::on_read(GattReadHandler handler) {
    read_handler_ = std::move(handler);
    ensure_gatt_handlers();
}

void BluetoothProxy::on_notify_data(GattNotifyDataHandler handler) {
    notify_data_handler_ = std::move(handler);
    ensure_gatt_handlers();
}

void BluetoothProxy::on_services(GattServicesHandler handler) {
    services_handler_ = std::move(handler);
    ensure_gatt_handlers();
}

void BluetoothProxy::on_services_done(GattServicesDoneHandler handler) {
    services_done_handler_ = std::move(handler);
    ensure_gatt_handlers();
}

void BluetoothProxy::on_error(GattErrorHandler handler) {
    error_handler_ = std::move(handler);
    ensure_gatt_handlers();
}

}  // namespace esphome::api
