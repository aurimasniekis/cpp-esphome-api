#pragma once

/// @file
/// @brief In-memory Transport double standing in for an ESPHome device. The
///        test plays the server: it completes the connect, delivers bytes to
///        the read pump and inspects everything the client wrote.

#include <esphome/api/bytes.hpp>
#include <esphome/api/transport/transport.hpp>

#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

namespace esphome::api::testing {

/// Transport implementation backed by test-controlled buffers.
class MockTransport final : public Transport {
public:
    // --- Transport interface -------------------------------------------------
    void async_connect(const std::string& connect_host,
                       const std::uint16_t connect_port,
                       ConnectHandler handler) override {
        connect_requested = true;
        host = connect_host;
        port = connect_port;
        connect_handler_ = std::move(handler);
    }

    void start_read(ReadHandler on_read) override {
        read_handler_ = std::move(on_read);
    }

    void async_write(ByteBuffer data, const WriteHandler handler) override {
        sent.insert(sent.end(), data.begin(), data.end());
        if (handler) {
            handler(std::error_code{});
        }
    }

    void close() override {
        open_ = false;
        closed = true;
    }

    [[nodiscard]] bool is_open() const override {
        return open_;
    }

    // --- Server-side test controls ------------------------------------------
    /// Complete the pending connect (success unless `ec` is set).
    void complete_connect(const std::error_code ec = {}) {
        open_ = !ec;
        if (connect_handler_) {
            const ConnectHandler handler = std::move(connect_handler_);
            connect_handler_ = nullptr;
            handler(ec);
        }
    }

    /// Deliver bytes to the client's read pump.
    void deliver(const ByteView bytes) const {
        if (read_handler_) {
            read_handler_(std::error_code{}, bytes);
        }
    }

    /// Signal a read error / EOF to the client.
    void deliver_error(const std::error_code ec) const {
        if (read_handler_) {
            read_handler_(ec, ByteView{});
        }
    }

    [[nodiscard]] bool has_read_handler() const noexcept {
        return static_cast<bool>(read_handler_);
    }

    // Observable state.
    bool connect_requested = false;
    bool closed = false;
    std::string host;
    std::uint16_t port = 0;
    ByteBuffer sent;

private:
    ConnectHandler connect_handler_;
    ReadHandler read_handler_;
    bool open_ = false;
};

}  // namespace esphome::api::testing
