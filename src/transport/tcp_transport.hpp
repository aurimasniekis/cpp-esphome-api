#pragma once

/// @file
/// @brief Internal Asio TCP socket wrapper. This header includes Asio and is
///        therefore private to the library (never installed / public).

#include <esphome/api/bytes.hpp>
#include <esphome/api/transport/transport.hpp>

#include <asio.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <system_error>

namespace esphome::api {

/// Thin async TCP socket: resolve+connect, a continuous read pump, and a
/// serialized write queue. All callbacks run on the owning io_context's
/// executor; the transport must be used from that executor (use a strand).
class TcpTransport final : public Transport {
public:
    explicit TcpTransport(const asio::any_io_executor& executor);
    ~TcpTransport() override;

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    void
    async_connect(const std::string& host, std::uint16_t port, ConnectHandler handler) override;
    void start_read(ReadHandler on_read) override;
    void async_write(ByteBuffer data, WriteHandler handler) override;
    void close() override;
    [[nodiscard]] bool is_open() const override;

private:
    void do_read();
    void do_write();

    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::array<std::uint8_t, 16384> read_buffer_{};
    ReadHandler read_handler_;

    struct PendingWrite {
        ByteBuffer data;
        WriteHandler handler;
    };
    std::deque<PendingWrite> write_queue_;
    bool writing_ = false;
};

}  // namespace esphome::api
