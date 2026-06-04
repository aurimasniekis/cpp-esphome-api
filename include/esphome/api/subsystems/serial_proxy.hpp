#pragma once

/// @file
/// @brief Serial (UART) proxy subsystem.

#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/subsystem.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace esphome::api {

/// UART parameters for a serial proxy instance (api: SerialProxyConfigureRequest).
struct SerialProxyConfig {
    std::uint32_t instance = 0;
    std::uint32_t baudrate = 0;
    bool flow_control = false;
    SerialProxyParity parity = SerialProxyParity::None;
    std::uint32_t stop_bits = 1;
    std::uint32_t data_size = 8;
};

/// Data forwarded from a serial device (api: SerialProxyDataReceived).
struct SerialProxyData {
    std::uint32_t instance = 0;
    std::string data;
};

/// Result of a SerialProxyRequest (api: SerialProxyRequestResponse).
struct SerialProxyResponse {
    std::uint32_t instance = 0;
    SerialProxyRequestType type = SerialProxyRequestType::Subscribe;
    SerialProxyStatus status = SerialProxyStatus::Ok;
    std::string error_message;
};

/// Typed view of the modem control/status lines exchanged with the proxy. Bit
/// positions are stable API (ESPHome `SerialProxyLineStateFlag`); only RTS and
/// DTR are defined today.
struct SerialProxyLineStates {
    bool rts = false;  ///< Request To Send (bit 0)
    bool dtr = false;  ///< Data Terminal Ready (bit 1)

    static constexpr std::uint32_t rts_bit = 1U << 0;
    static constexpr std::uint32_t dtr_bit = 1U << 1;

    [[nodiscard]] static SerialProxyLineStates from_bits(const std::uint32_t bits) {
        return {(bits & rts_bit) != 0, (bits & dtr_bit) != 0};
    }
    [[nodiscard]] std::uint32_t to_bits() const {
        return (rts ? rts_bit : 0U) | (dtr ? dtr_bit : 0U);
    }
};

/// Current modem control pin states (api: SerialProxyGetModemPinsResponse).
struct SerialProxyModemPins {
    std::uint32_t instance = 0;
    SerialProxyLineStates lines;
};

class SerialPort;

/// Proxies a UART on the device, forwarding data and control to clients.
class SerialProxy : public Subsystem {
public:
    using DataHandler = std::function<void(const SerialProxyData&)>;
    using ResponseHandler = std::function<void(const SerialProxyResponse&)>;
    using ModemPinsHandler = std::function<void(const SerialProxyModemPins&)>;

    explicit SerialProxy(Client& client) : Subsystem(client) {}

    /// An instance-bound handle so you don't repeat the instance index on every
    /// call: `client.serial_proxy(0).write(...)`.
    [[nodiscard]] SerialPort instance(std::uint32_t index);

    /// Register a callback fired for every incoming SerialProxyDataReceived
    /// (any instance).
    void on_data(DataHandler handler);

    /// Register a callback fired only for data from `instance`.
    void on_data(std::uint32_t instance, DataHandler handler);

    /// Register a callback fired for each SerialProxyRequestResponse.
    void on_request_response(ResponseHandler handler);

    /// Register a persistent callback fired for every modem-pin response.
    void on_modem_pins(ModemPinsHandler handler);

    /// Issue a simple control request (subscribe, unsubscribe, flush).
    void request(std::uint32_t instance, SerialProxyRequestType type) const;

    /// Configure UART parameters for an instance.
    void configure(const SerialProxyConfig& config) const;

    /// Write raw data to the serial device.
    void write(std::uint32_t instance, const std::string& data) const;

    /// Request the current modem control pin states. If `once` is provided it is
    /// invoked exactly once with the matching response for this instance (plus
    /// any persistent on_modem_pins handler). Drive the event loop (e.g.
    /// SyncClient::pump_until) to receive it.
    void get_modem_pins(std::uint32_t instance, ModemPinsHandler once = {});

    /// Set modem control pin states (RTS and DTR).
    void set_modem_pins(std::uint32_t instance, SerialProxyLineStates lines) const;

    /// Set modem control pin states from a raw bitmask (advanced).
    void set_modem_pins_raw(std::uint32_t instance, std::uint32_t line_states) const;

private:
    void ensure_modem_pins_handler();
    void ensure_data_handler();

    DataHandler data_handler_;
    std::unordered_map<std::uint32_t, DataHandler> instance_data_handlers_;
    ResponseHandler response_handler_;
    ModemPinsHandler modem_pins_handler_;
    std::unordered_map<std::uint32_t, std::vector<ModemPinsHandler>> pending_modem_pins_;
    bool data_registered_ = false;
    bool modem_pins_registered_ = false;
};

/// A single serial-proxy instance bound to its index — the instance-aware,
/// ergonomic front end to SerialProxy (no instance argument on every call).
class SerialPort {
public:
    SerialPort(SerialProxy& proxy, const std::uint32_t instance)
        : proxy_(&proxy), instance_(instance) {}

    [[nodiscard]] std::uint32_t index() const noexcept {
        return instance_;
    }

    void subscribe() const {
        proxy_->request(instance_, SerialProxyRequestType::Subscribe);
    }
    void unsubscribe() const {
        proxy_->request(instance_, SerialProxyRequestType::Unsubscribe);
    }
    void flush() const {
        proxy_->request(instance_, SerialProxyRequestType::Flush);
    }

    void configure(SerialProxyConfig config) const {
        config.instance = instance_;
        proxy_->configure(config);
    }
    void write(const std::string& data) const {
        proxy_->write(instance_, data);
    }

    void get_modem_pins(SerialProxy::ModemPinsHandler once = {}) const {
        proxy_->get_modem_pins(instance_, std::move(once));
    }
    void set_modem_pins(const SerialProxyLineStates lines) const {
        proxy_->set_modem_pins(instance_, lines);
    }

    /// Receive data for this instance only.
    void on_data(SerialProxy::DataHandler handler) const {
        proxy_->on_data(instance_, std::move(handler));
    }

private:
    SerialProxy* proxy_;
    std::uint32_t instance_;
};

inline SerialPort SerialProxy::instance(const std::uint32_t index) {
    return SerialPort(*this, index);
}

}  // namespace esphome::api
